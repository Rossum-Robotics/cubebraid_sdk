#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <iomanip>
#include <atomic>
#include <mutex>

#include <opencv2/opencv.hpp>
#include "CubeBraidSDK/DL_SDK/DlSegSdk.h"

/**
 * @brief 作用域计时器类：用于精准测量代码块执行耗时并在析构时打印
 */
class ScopedTimer
{
public:
    explicit ScopedTimer(const char* name)
        : name_(name),
          start_(std::chrono::steady_clock::now())
    {
    }

    ~ScopedTimer()
    {
        const auto end = std::chrono::steady_clock::now();

        const double ms =
            std::chrono::duration<double, std::milli>(
                end - start_).count();

        std::cout << "[DlSegTimer] "
                  << name_ << ": "
                  << std::fixed << std::setprecision(3)
                  << ms << " ms"
                  << std::endl;
    }

private:
    const char* name_;
    std::chrono::steady_clock::time_point start_;
};

int main()
{
    std::cout << "=== 启动 DlSegSdk 摄像头实时实例分割 Demo ===" << std::endl;

    // 实例化 SDK 对象
    dlseg_sdk::DlSegSdk segmenter;

    // 配置 SDK 初始化参数
    dlseg_sdk::SdkConfig config;
    config.model_path = "./data/models/unload.onnx";
    config.class_names_path = "./data/models/classes.txt"; 

    config.use_cuda = true;        // 是否开启 CUDA 加速
    config.cuda_id = 0;
    config.warm_up = true;

    config.net_width = 640;         // 模型输入宽度
    config.net_height = 640;        // 模型输入高度
    config.class_threshold = 0.25f; // 置信度阈值
    config.nms_threshold = 0.45f;   // NMS 阈值
    config.mask_threshold = 0.5f;   // 掩膜阈值
    config.batch_size = 1;

    // 异步初始化 SDK
    dlseg_sdk::StatusCode status = segmenter.initialize(config);
    if (status != dlseg_sdk::StatusCode::SUCCESS)
    {
        std::cerr << "初始化接口调用失败，错误信息: " << segmenter.getLastError() << std::endl;
        return -1;
    }

    // 等待模型后台异步加载完成
    std::cout << "正在等待后台异步加载模型与初始化..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2)); 

    // 打开电脑摄像头（通常 0 代表默认摄像头）
    cv::VideoCapture cap(0);
    if (!cap.isOpened())
    {
        std::cerr << "错误：无法打开电脑摄像头！请检查设备连接或摄像头权限。" << std::endl;
        return -1;
    }

    // 设置摄像头分辨率（可根据需要调整）
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 720);

    std::cout << "摄像头已成功打开，按键盘上的 'ESC' 键可以退出实时检测。" << std::endl;

    // 线程控制变量
    std::atomic<bool> g_running(true);
    std::mutex mtx;
    cv::Mat current_frame;
    cv::Mat latest_canvas;
    bool has_new_frame = false;

    // 创建独立线程用于循环捕获和实时推理
    std::thread capture_detect_thread([&]() {
        while (g_running)
        {
            cv::Mat frame;
            {
                std::lock_guard<std::mutex> lock(mtx);
                if (current_frame.empty())
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    continue;
                }
                frame = current_frame.clone(); // 拷贝一帧用于检测
            }

            // 执行实例分割推理
            std::vector<dlseg_sdk::OutputParams> results;
            cv::Mat canvas;
            cv::Mat mask_img;

            {
                ScopedTimer timer("摄像头单帧实例分割耗时");
                status = segmenter.detect(frame, results, canvas, mask_img);
            }

            if (status == dlseg_sdk::StatusCode::SUCCESS)
            {
                std::lock_guard<std::mutex> lock(mtx);
                if (!canvas.empty())
                {
                    latest_canvas = canvas.clone();
                    has_new_frame = true;
                }
            }
            else
            {
                std::cerr << "推理失败，状态码: " << static_cast<int>(status) 
                          << " (" << dlseg_sdk::statusToString(status) << ")" << std::endl;
            }
        }
    });

    // 主线程负责从摄像头读取画面，并刷新显示窗口
    while (g_running)
    {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty())
        {
            std::cerr << "警告：捕获到空视频帧！" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // 将读取到的最新帧更新给工作线程
        {
            std::lock_guard<std::mutex> lock(mtx);
            current_frame = frame;
        }

        // 显示带有检测和渲染结果的画布
        cv::Mat display_mat;
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (has_new_frame && !latest_canvas.empty())
            {
                display_mat = latest_canvas.clone();
            }
            else
            {
                display_mat = frame; // 还没有推理出结果前先展示原图
            }
        }

        cv::imshow("DlSegSdk 实时摄像头检测", display_mat);

        // 检测键盘输入，按下 ESC (键值 27) 退出
        char key = static_cast<char>(cv::waitKey(1));
        if (key == 27)
        {
            g_running = false;
            break;
        }
    }

    // 5. 回收线程资源与释放摄像头
    if (capture_detect_thread.joinable())
    {
        capture_detect_thread.join();
    }

    cap.release();
    cv::destroyAllWindows();

    std::cout << "=== 实时检测 Demo 运行结束 ===" << std::endl;
    return 0;
}