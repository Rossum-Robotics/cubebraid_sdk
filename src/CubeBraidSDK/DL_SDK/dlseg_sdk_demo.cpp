#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>
#include "CubeBraidSDK/DL_SDK/DlSegSdk.h"

int main()
{
    // 创建 SDK 实例
    dlseg_sdk::DlSegSdk segmenter;

    // 配置初始化参数
    dlseg_sdk::SdkConfig config;
    config.model_path = "./data/models/unload.onnx";
    config.use_cuda = true;
    config.cuda_id = 0;
    config.warm_up = true;

    // 初始化模型
    dlseg_sdk::StatusCode status = segmenter.initialize(config);
    if (status != dlseg_sdk::StatusCode::SUCCESS)
    {
        std::cerr << "模型初始化失败，错误信息: " << segmenter.getLastError() << std::endl;
        return -1;
    }
    std::cout << "模型初始化成功！" << std::endl;

    // 读取测试图像
    cv::Mat test_img = cv::imread("./data/img/demo.png");
    if (test_img.empty())
    {
        std::cerr << "无法读取测试图像文件！" << std::endl;
        return -1;
    }

    // 执行 ONNX 检测与分割推理
    std::vector<dlseg_sdk::OutputParams> results;
    std::string save_path = "./data/img/output_masks/result_mask.png";
    status = segmenter.detect(test_img, results, save_path); // 传入路径即自动保存
    // status = segmenter.detect(test_img, results);
    
    if (status == dlseg_sdk::StatusCode::SUCCESS)
    {
        std::cout << "推理成功，检测到目标数量: " << results.size() << std::endl;
        for (size_t i = 0; i < results.size(); ++i)
        {
            std::cout << "  - 目标 [" << i << "] 类别ID: " << results[i].id 
                      << ", 置信度: " << results[i].confidence 
                      << ", 边界框: [" << results[i].box.x << ", " << results[i].box.y 
                      << ", " << results[i].box.width << ", " << results[i].box.height << "]" << std::endl;
        }

        // 自定义路径和文件名保存全部掩膜结果
        std::string custom_path = "./data/img/output_masks";
        std::string custom_name = "custom_segmented_mask.png";
        
        auto status = segmenter.SaveMask(test_img, results, custom_path, custom_name);
        if (status == dlseg_sdk::StatusCode::SUCCESS)
        {
            std::cout << "掩膜保存成功，路径: " << custom_path + "/" + custom_name << std::endl;
        }
        else
        {
            std::cerr << "掩膜保存失败，错误码: " << static_cast<int>(status) << std::endl;
        }
    }
    else
    {
        std::cerr << "推理失败，状态码: " << static_cast<int>(status) 
                  << " 错误: " << segmenter.getLastError() << std::endl;
    }

    return 0;
}