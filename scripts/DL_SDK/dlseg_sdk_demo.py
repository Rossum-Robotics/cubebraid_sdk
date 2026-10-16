import os
import cv2
from dlseg_sdk import DlSegSDK

def main():
    print("=== DlSeg Python SDK Demo 程序启动 ===")
    model_path = "./data/models/unload.onnx"
    test_img_path = "./data/img/demo.png"
    output_save_path = "./data/img/output_masks/python_rotated_mask_result.png"
    
    # 独立保存纯掩膜的目录及文件名配置
    mask_save_dir = "./data/img/output_masks"
    mask_file_name = "pure_segmentation_mask.png"

    # 读取测试图像
    img = cv2.imread(test_img_path)
    if img is None:
        print(f"[错误] 无法读取图像: {test_img_path}")
        return

    try:
        with DlSegSDK() as segmenter:
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

            # 执行检测推理，并自动在图上绘制旋转框、标签和掩膜并保存
            count, boxes = segmenter.detect(
                img_np=img,
                save_path=output_save_path,
                max_boxes=50
            )

            if count >= 0:
                print(f"[成功] 推理成功，检测到目标数量: {count}")
                print(f"带有倾斜旋转框及掩膜的结果图片已成功保存至: {output_save_path}")
                
                for i, box in enumerate(boxes):
                    print(f"  - 目标 [{i}] 类别ID: {box.id}, 置信度: {box.confidence:.3f}, 区域: [x={box.x}, y={box.y}, w={box.width}, h={box.height}]")

                # 提取并保存独立的分割掩膜图片
                os.makedirs(mask_save_dir, exist_ok=True)
                mask_status = segmenter.save_mask(
                    img_np=img,
                    boxes=boxes,
                    save_path=mask_save_dir,
                    file_name=mask_file_name
                )
                
                if mask_status == 0:
                    print(f"[成功] 独立提取的掩膜图片已成功保存至: {os.path.join(mask_save_dir, mask_file_name)}")
                else:
                    print(f"[警告] 独立保存掩膜图片失败，状态码: {mask_status}")

            else:
                print(f"[错误] 推理执行失败，错误码: {count}, 错误信息: {segmenter.get_last_error()}")

    except RuntimeError as e:
        print(f"[运行时错误] {e}")
    except Exception as e:
        print(f"[未知异常] {e}")

    print("=== Demo 退出 ===")

if __name__ == "__main__":
    main()