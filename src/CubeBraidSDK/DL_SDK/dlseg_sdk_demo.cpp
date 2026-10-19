#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

#include <opencv2/opencv.hpp>
#include "CubeBraidSDK/DL_SDK/DlSegSdk.h"

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
    dlseg_sdk::DlSegSdk segmenter;

    dlseg_sdk::SdkConfig config;
    config.model_path = "./data/models/unload.onnx";
    
    // 指定类别名称文件的路径（请确保该文件和模型在对应目录下或路径正确）
    config.class_names_path = "./data/models/classes.txt"; 

    config.use_cuda = true;
    config.cuda_id = 0;
    config.warm_up = true;

    // 根据模型实际训练情况调整
    config.net_width = 320;        // 如果模型是320x320训练的，请设为320（默认 640）
    config.net_height = 320;       // 同上
    config.class_threshold = 0.25f; // 适当调低置信度阈值（默认 0.25f）防漏检
    config.nms_threshold = 0.45f;
    config.mask_threshold = 0.5f;
    config.batch_size = 1;

    dlseg_sdk::StatusCode status = segmenter.initialize(config);
    if (status != dlseg_sdk::StatusCode::SUCCESS)
    {
        std::cerr << "模型初始化失败，错误信息: " << segmenter.getLastError() << std::endl;
        return -1;
    }
    
    cv::Mat test_img = cv::imread("./data/img/demo.png");
    if (test_img.empty())
    {
        std::cerr << "无法读取测试图像文件！" << std::endl;
        return -1;
    }

    // 延时 2 秒
    std::this_thread::sleep_for(std::chrono::seconds(2)); // 为了计算实例分割耗时等待模型初始化完成（异步线程不阻塞，正常使用不需要等待）

    std::vector<dlseg_sdk::OutputParams> results;
    cv::Mat canvas;   // 接收渲染画布
    cv::Mat mask_img; // 接收单通道全局纯净掩膜
    {
        ScopedTimer timer("实例分割检测时间");
        // 调用更新后的 detect 接口，同时获取 canvas 和 mask_img
        status = segmenter.detect(test_img, results, canvas, mask_img);
    }
    
    if (status == dlseg_sdk::StatusCode::SUCCESS)
    {
        std::cout << "推理成功，检测到目标数量: " << results.size() << std::endl;

        // 打印每个检测到的目标实际类名与置信度，供控制台查阅
        for (size_t i = 0; i < results.size(); ++i)
        {
            std::cout << " - 目标 [" << i << "] 类别ID: " << results[i].id 
                      << " 置信度: " << results[i].confidence * 100 << "%" << std::endl;
        }

        // 保存渲染好的可视化结果图（图片上的文字标签现在会自动显示实际类名）
        if (!canvas.empty())
        {
            std::string save_path = "./data/img/output_masks/result_mask.png";
            cv::imwrite(save_path, canvas);
            std::cout << "渲染结果图片已成功保存至: " << save_path << std::endl;
        }

        // 保存全局纯净掩膜图，方便后续直接调用处理
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
                  << " 错误: " << segmenter.getLastError() << std::endl;
    }

    return 0;
}