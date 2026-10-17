import os
import cv2
import time

from dlseg_sdk import DlSegSDK

def main():
    print("=== DlSeg Python SDK Demo 程序启动 ===")
    model_path = "./data/models/unload.onnx"
    test_img_path = "./data/img/testimg.jpg"
    output_save_path = "./data/img/output_masks/python_rotated_mask_result.png"
    
    # 纯净全局掩膜的保存路径及文件名配置
    mask_save_dir = "./data/img/output_masks"
    mask_file_name = "global_pure_segmentation_mask.png"

    # 读取测试图像
    img = cv2.imread(test_img_path)
    if img is None:
        print(f"[错误] 无法读取图像: {test_img_path}")
        return

    try:
        with DlSegSDK() as segmenter:
            start = time.perf_counter()
            # 初始化模型配置
            ret = segmenter.initialize(
                model_path=model_path,
                use_cuda=1,
                cuda_id=0,
                warm_up=1
            )
            if ret != 0:
                print(f"[错误] 模型初始化失败，状态码: {ret}, 错误信息: {segmenter.get_last_error()}")
                return
            print("[成功] 模型初始化成功！")
            
            end = time.perf_counter()
            print(f"模型初始化耗时: {(end - start) * 1000:.3f} 毫秒")

            start = time.perf_counter()
            
            # 调用升级后的 detect 接口，同时获取渲染画布 (rendered_img) 和纯净全局掩膜 (mask_img)
            count, boxes, rendered_img, mask_img = segmenter.detect(
                img_np=img,
                max_boxes=100
            )
            
            end = time.perf_counter()
            print(f"检测推理与渲染总耗时: {(end - start) * 1000:.3f} 毫秒 (主线程丝滑无卡顿)")

            if count >= 0:
                print(f"[成功] 推理成功，检测到目标数量: {count}")
                
                # 保存带有旋转框及半透明掩膜的可视化结果图
                if rendered_img is not None:
                    os.makedirs(os.path.dirname(output_save_path), exist_ok=True)
                    cv2.imwrite(output_save_path, rendered_img)
                    print(f"带有倾斜旋转框及掩膜的结果图片已成功保存至: {output_save_path}")
                
                # 直接利用 SDK 高效返回的 mask_img 保存全局纯净掩膜图片
                if mask_img is not None:
                    os.makedirs(mask_save_dir, exist_ok=True)
                    full_mask_path = os.path.join(mask_save_dir, mask_file_name)
                    cv2.imwrite(full_mask_path, mask_img)
                    print(f"[成功] 全局纯净掩膜图片已直接保存至: {full_mask_path}")

            else:
                print(f"[错误] 推理执行失败，错误码: {count}, 错误信息: {segmenter.get_last_error()}")

    except RuntimeError as e:
        print(f"[运行时错误] {e}")
    except Exception as e:
        print(f"[未知异常] {e}")

    print("=== Demo 退出 ===")

if __name__ == "__main__":
    main()