#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <iomanip>

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
    std::cout << "=== 启动 DlSegSdk 实例分割测试 Demo ===" << std::endl;

    // 实例化 SDK 对象
    dlseg_sdk::DlSegSdk segmenter;

    // 配置 SDK 初始化参数
    dlseg_sdk::SdkConfig config;
    config.model_path = "./data/models/unload.onnx";
    
    // 指定类别名称文件的路径（请确保该文件和模型在对应目录下或路径正确）
    config.class_names_path = "./data/models/classes.txt"; 

    config.use_cuda = true;        // 是否开启 CUDA 加速
    config.cuda_id = 0;
    config.warm_up = true;

    // 根据模型实际训练情况调整
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

    // 读取测试图像
    std::string test_img_path = "./data/img/demo.png";
    cv::Mat test_img = cv::imread(test_img_path);
    if (test_img.empty())
    {
        std::cerr << "无法读取测试图像文件！路径: " << test_img_path << std::endl;
        std::cerr << "请确保在运行 Demo 的目录下存在该图片，或修改为正确的绝对路径。" << std::endl;
        return -1;
    }

    // 等待模型后台异步加载完成
    // 注：`detect` 内部会自动安全等待异步初始化任务，此处 sleep 仅为演示和纯测试推理耗时
    std::cout << "正在等待后台异步加载模型与初始化..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2)); 

    // 6. 执行实例分割推理
    std::vector<dlseg_sdk::OutputParams> results;
    cv::Mat canvas;   // 接收渲染画布
    cv::Mat mask_img; // 接收单通道全局纯净掩膜
    
    {
        ScopedTimer timer("实例分割检测总耗时");
        // 调用 detect 接口，同时获取 canvas 和 mask_img（内部会阻塞等待异步初始化完毕）
        status = segmenter.detect(test_img, results, canvas, mask_img);
    }
    
    // 处理推理结果
    if (status == dlseg_sdk::StatusCode::SUCCESS)
    {
        std::cout << "推理成功，检测到目标数量: " << results.size() << std::endl;

        for (size_t i = 0; i < results.size(); ++i)
        {
            std::cout << " - 目标 [" << i << "] 类别ID: " << results[i].id 
                      << " 置信度: " << results[i].confidence * 100 << "%" << std::endl;
        }

        // 保存渲染好的可视化结果图
        if (!canvas.empty())
        {
            std::string save_path = "./data/img/output_masks/result_mask.png";
            cv::imwrite(save_path, canvas);
            std::cout << "渲染结果图片已成功保存至: " << save_path << std::endl;
        }

        // 保存全局纯净掩膜图
        if (!mask_img.empty())
        {
            std::string mask_path = "./data/img/output_masks/global_pure_mask.png";
            cv::imwrite(mask_path, mask_img);
            std::cout << "全局纯净掩膜图已成功保存至: " << mask_path << std::endl;
        }
    }
    else
    {
        std::cerr << "推理失败，状态码: " << static_cast<int>(status) 
                  << " (" << dlseg_sdk::statusToString(status) << ")"
                  << " 错误详情: " << segmenter.getLastError() << std::endl;
    }

    std::cout << "=== Demo 运行结束 ===" << std::endl;
    return 0;
}