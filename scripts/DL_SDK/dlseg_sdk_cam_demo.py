import os
import cv2
import numpy as np
from dlseg_sdk import DlSegSDK

def main():
    print("=== DlSeg Python SDK 摄像头实时识别 Demo 启动 ===")
    model_path = "./data/models/unload.onnx"
    
    # 初始化摄像头（0 代表默认内置/外接摄像头，如果是rtsp或视频流也可以填入对应的字符串或地址）
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("[错误] 无法打开摄像头，请检查设备连接或索引！")
        return

    # 设置摄像头分辨率（可根据需要调整）
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)

    # 类别颜色盘（用于不同类别显示不同颜色 BGR）
    colors = [
        (255, 0, 0),   (0, 255, 0),   (0, 0, 255),
        (255, 255, 0), (255, 0, 255), (0, 255, 255)
    ]

    try:
        with DlSegSDK() as segmenter:
            # 在循环外部初始化模型（仅加载一次）
            print("[信息] 正在初始化模型，请稍候...")
            ret = segmenter.initialize(
                model_path=model_path,
                use_cuda=1,
                cuda_id=0,
                warm_up=1
            )
            if ret != 0:
                print(f"[错误] 模型初始化失败，状态码: {ret}, 错误信息: {segmenter.get_last_error()}")
                cap.release()
                return
            print("[成功] 模型初始化成功！开始实时推理，按 'q' 键或 'ESC' 退出。")

            while True:
                ret, frame = cap.read()
                if not ret:
                    print("[警告] 无法获取摄像头帧，跳过...")
                    break

                # 执行检测推理（实时流不传 save_path，避免磁盘 IO 拖慢帧率）
                count, boxes = segmenter.detect(
                    img_np=frame,
                    save_path="", 
                    max_boxes=100
                )

                if count >= 0:
                    canvas = frame.copy()
                    mask_overlay = canvas.copy()
                    has_mask = False

                    for box in boxes:
                        color = colors[box.id % len(colors)]
                        
                        # 绘制水平矩形框 (如果 SDK 支持 rotatedBox，也可以根据需要调整)
                        cv2.rectangle(canvas, (box.x, box.y), (box.x + box.width, box.y + box.height), color, 2, cv2.LINE_AA)
                        
                        # 绘制文本标签（类别ID + 置信度）
                        label = f"ID:{box.id} {int(box.confidence * 100)}%"
                        top_pos = max(box.y, 20)
                        cv2.putText(canvas, label, (box.x, top_pos - 5), cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 1, cv2.LINE_AA)

                        # 如果有 Mask 数据，将其叠加到预览画面中
                        if hasattr(box, 'boxMask') and box.boxMask is not None and box.boxMask.size > 0:
                            roi_h, roi_w = box.boxMask.shape[:2]
                            if box.x >= 0 and box.y >= 0 and box.x + roi_w <= canvas.shape[1] and box.y + roi_h <= canvas.shape[0]:
                                roi = mask_overlay[box.y:box.y + roi_h, box.x:box.x + roi_w]
                                if roi.shape[:2] == box.boxMask.shape[:2]:
                                    roi[box.boxMask > 0] = color
                                    has_mask = True

                    # 如果存在掩膜，将掩膜图层与原画布进行半透明加权融合
                    if has_mask:
                        cv2.addWeighted(canvas, 0.5, mask_overlay, 0.5, 0, canvas)

                    # 在画面上实时显示检测到的目标总数
                    cv2.putText(canvas, f"Targets: {count}", (30, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)
                    
                    # 实时展示画面
                    cv2.imshow("DlSeg Camera Realtime Demo", canvas)
                else:
                    # 推理失败时显示原画面并打印错误
                    cv2.imshow("DlSeg Camera Realtime Demo", frame)

                # 监听键盘按键：按 'q' 或 'ESC' (ASCII 27) 退出循环
                key = cv2.waitKey(1) & 0xFF
                if key == ord('q') or key == 27:
                    print("[信息] 用户请求退出...")
                    break

    except RuntimeError as e:
        print(f"[运行时错误] {e}")
    except Exception as e:
        print(f"[未知异常] {e}")
    finally:
        # 释放摄像头及销毁 OpenCV 窗口
        cap.release()
        cv2.destroyAllWindows()

    print("=== Demo 退出 ===")

if __name__ == "__main__":
    main()