#ifndef DL_SEG_SDK_H
#define DL_SEG_SDK_H

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

// 跨平台动态库导出/导入宏定义
#ifdef _WIN32
    #ifdef DLSEG_SDK_EXPORTS
        #define DLSEG_API __declspec(dllexport)
    #else
        #define DLSEG_API __declspec(dllimport)
    #endif
#else
    #define DLSEG_API __attribute__((visibility("default")))
#endif

namespace dlseg_sdk
{
    /**
     * @brief SDK 执行状态码
     */
    enum class StatusCode
    {
        SUCCESS = 0,               // 执行成功
        MODEL_LOAD_FAILED = 1,     // 模型加载失败
        INFERENCE_FAILED = 2,      // 模型推理计算失败
        INVALID_PARAMETER = 3,     // 传入的参数无效或错误
        POINT_CLOUD_FAILED = 4,  // 点云处理相关错误
        ALGORITHM_FAILED = 5,    // 算法内部计算或逻辑失败
        INTERNAL_ERROR = 99        // 未知的内部严重错误
    };

    /**
     * @brief 检测与分割结果参数结构体
     */
    struct OutputParams
    {
        int id;                      // 结果类别 ID
        float confidence;            // 置信度
        cv::Rect box;                // 矩形框
        cv::RotatedRect rotatedBox;  // OBB 倾斜旋转矩形框（支持物体跟随旋转）
        cv::Mat boxMask;             // 矩形框内 mask 掩膜
    };

    /**
     * @brief SDK 初始化配置参数
     */
    struct SdkConfig
    {
        std::string model_path;      // 模型路径
        std::string class_names_path; // 类别名称文件路径（每行一个类名）
        bool use_cuda = true;        // 是否使用 CUDA
        int cuda_id = 0;             // CUDA 设备 ID
        bool warm_up = true;         // 是否在初始化时执行模型预热
    };

    /**
     * @brief 深度学习实例分割推理SDK 
     */
    class DLSEG_API DlSegSdk
    {
    public:
        DlSegSdk();
        ~DlSegSdk();

        // 禁用拷贝构造函数和拷贝赋值运算符，防止底层资源重复释放
        DlSegSdk(const DlSegSdk&) = delete;
        DlSegSdk& operator=(const DlSegSdk&) = delete;

        // 显式支持移动构造函数和移动赋值运算符
        DlSegSdk(DlSegSdk&&) noexcept;
        DlSegSdk& operator=(DlSegSdk&&) noexcept;

        /**
         * @brief 初始化并加载 ONNX 模型
         * @param config 初始化配置
         * @return 状态码
         */
        StatusCode initialize(const SdkConfig& config);

        /**
         * @brief 执行实例分割检测主接口（同时输出可视化画布与纯净掩膜图像）
         * @param img 输入原图
         * @param output 检测与分割结果参数列表
         * @param canvas 输出参数：渲染好的可视化画布（含框、Mask、标签）
         * @param mask_img 输出参数：整张图的纯净分割掩膜图像（单通道灰度或全黑底色，目标区域为白色/ID色）
         */
        StatusCode detect(cv::Mat& img, std::vector<OutputParams>& output, cv::Mat& canvas, cv::Mat& mask_img);

        /**
        * @brief 提取并保存识别结果的全部掩膜图片
        * @param img 输入图像
        * @param result detect 函数返回的识别结果数组
        * @param save_path 保存目录路径 (例如 "./data/img/")
        * @param file_name 保存的文件名 (例如 "mask_result.png")
        * @return 状态码 (StatusCode::SUCCESS 表示成功)
        */
        StatusCode SaveMask(cv::Mat& img, const std::vector<OutputParams>& result, const std::string& save_path, const std::string& file_name);

        /**
         * @brief 获取最后一次发生的详细错误描述字符串
         * @return std::string 错误信息
         */
        std::string getLastError() const;

        /**
         * @brief 获取最后一次操作的底层状态码
         * @return StatusCode 状态码
         */
        StatusCode getLastStatus() const noexcept;

    private:
        class Impl;
        Impl* impl_;
    };

    /**
     * @brief 将状态码枚举转换并输出为对应的可读字符串
     * @param status 状态码枚举值
     * @return std::string 状态描述文本
     */
    DLSEG_API std::string statusToString(StatusCode status);
}

#endif // DL_SEG_SDK_H