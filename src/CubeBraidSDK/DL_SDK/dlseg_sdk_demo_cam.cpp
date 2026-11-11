#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <iomanip>
#include <atomic>
#include <mutex>
#include <cstdlib>
#include <ctime>

#include <opencv2/opencv.hpp>
#include "CubeBraidSDK/DL_SDK/DlSegSdk.h"

// 线程与数据同步相关全局变量
cv::Mat g_raw_frame;
std::mutex g_frame_mutex;        // 保护摄像头原始帧的互斥锁

std::atomic<bool> g_running(true);
std::mutex g_canvas_mutex;       // 保护渲染画布的互斥锁
cv::Mat latest_canvas;

// 实例化 SDK 对象
dlseg_sdk::DlSegSdk segmenter;
dlseg_sdk::SdkConfig config;
dlseg_sdk::StatusCode dl_status;

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
        const double ms = std::chrono::duration<double, std::milli>(end - start_).count();
        if (ms > 100)
        {
            std::cout << "[DlSegTimer] " << name_ << ": " << std::fixed << std::setprecision(3) << ms << " ms" << std::endl;

        }
        
    }

private:
    const char* name_;
    std::chrono::steady_clock::time_point start_;
};

// 异步检测与识别线程函数
void RecognitionThread()
{
    while (g_running)
    {
        cv::Mat img_to_process;
        
        // 1. 安全地从全局获取一帧并深拷贝（避免多线程冲突）
        {
            std::lock_guard<std::mutex> lock(g_frame_mutex);
            if (g_raw_frame.empty() || g_raw_frame.cols <= 0 || g_raw_frame.rows <= 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            img_to_process = g_raw_frame.clone();
        }

        // 2. 按照你的要求：获取图片后随机做缩放
        double scale_factors[] = {0.5, 0.75, 1.0, 1.25, 1.5};
        int rand_idx = std::rand() % 5;
        double scale = scale_factors[rand_idx];

        int target_w = static_cast<int>(img_to_process.cols * scale);
        int target_h = static_cast<int>(img_to_process.rows * scale);
        if (target_w < 64) target_w = 64;
        if (target_h < 64) target_h = 64;

        cv::Mat img_copy;
        cv::resize(img_to_process, img_copy, cv::Size(target_w, target_h), 0, 0, cv::INTER_LINEAR);

        // 3. 执行 ONNX 检测与分割推理
        std::vector<dlseg_sdk::OutputParams> dl_results;
        cv::Mat canvas;   
        cv::Mat mask_img; 

        {
            ScopedTimer timer("实例分割总耗时");
            dl_status = segmenter.detect(img_copy, dl_results, canvas, mask_img);
        }

        // 4. 安全更新渲染画布
        if (dl_status == dlseg_sdk::StatusCode::SUCCESS)
        {
            std::lock_guard<std::mutex> lock(g_canvas_mutex);
            if (!canvas.empty())
            {
                latest_canvas = canvas.clone();
            }
        }

        // 适当休眠，避免单线程过载
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}

int main()
{
    std::cout << "=== 启动 DlSegSdk 摄像头实时实例分割 Demo ===" << std::endl;

    // 初始化随机数种子
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    config.model_path = "./data/models/unload.onnx";
    config.class_names_path = "./data/models/classes.txt"; 
    config.use_cuda = true;        
    config.cuda_id = 0;
    config.warm_up = true;
    config.net_width = 640;         
    config.net_height = 640;        
    config.class_threshold = 0.25f; 
    config.nms_threshold = 0.45f;   
    config.mask_threshold = 0.5f;   
    config.batch_size = 1;

    // 异步初始化 SDK
    dlseg_sdk::StatusCode status = segmenter.initialize(config);
    if (status != dlseg_sdk::StatusCode::SUCCESS)
    {
        std::cerr << "初始化接口调用失败，错误信息: " << segmenter.getLastError() << std::endl;
        return -1;
    }

    std::cout << "正在等待后台异步加载模型与初始化..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2)); 

    // 打开电脑摄像头
    cv::VideoCapture cap(0, cv::CAP_MSMF);
    if (!cap.isOpened())
    {
        std::cerr << "错误：无法打开电脑摄像头！" << std::endl;
        return -1;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
    std::cout << "摄像头已成功打开，按键盘上的 'ESC' 键可以退出实时检测。" << std::endl;

    // 启动视觉识别线程
    g_running = true;
    std::thread recog_thread(RecognitionThread);

    // 主线程负责从摄像头读取画面，并刷新显示窗口
    while (g_running)
    {
        cv::Mat temp_frame;
        cap >> temp_frame;
        
        if (temp_frame.empty() || temp_frame.cols <= 0 || temp_frame.rows <= 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // 安全地将摄像头新帧写入全局变量
        {
            std::lock_guard<std::mutex> lock(g_frame_mutex);
            g_raw_frame = temp_frame.clone();
        }

        // 获取显示内容
        cv::Mat display_mat;
        {
            std::lock_guard<std::mutex> lock(g_canvas_mutex);
            if (!latest_canvas.empty())
            {
                display_mat = latest_canvas.clone();
            }
            else
            {
                std::lock_guard<std::mutex> lock(g_frame_mutex);
                display_mat = g_raw_frame.clone(); // 还没有推理出结果前先展示原图
            }
        }

        if (!display_mat.empty())
        {
            cv::imshow("DlSegSdk 实时摄像头检测", display_mat);
        }

        // 检测键盘输入，按下 ESC (键值 27) 退出
        char key = static_cast<char>(cv::waitKey(1));
        if (key == 27)
        {
            g_running = false;
            break;
        }
    }

    // 回收线程资源与释放摄像头
    if (recog_thread.joinable())
    {
        recog_thread.join();
    }

    cap.release();
    cv::destroyAllWindows();

    std::cout << "=== 实时检测 Demo 运行结束 ===" << std::endl;
    return 0;
}