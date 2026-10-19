import cv2
import time
from dlseg_sdk import DlSegSDK

def main():
    print("=== DlSeg Python SDK 摄像头实时识别 Demo 启动 ===")
    model_path = "./data/models/yolov8m-seg.onnx"
    class_path = "./data/models/classes_yolo.txt"  # 指定类别文件名路径
    
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("[错误] 无法打开摄像头！")
        return

    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)

    try:
        with DlSegSDK() as segmenter:
            print("[信息] 正在初始化模型与类别标签...")
            # 【修改】传入 class_names_path 参数
            ret = segmenter.initialize(model_path=model_path, class_names_path=class_path, use_cuda=1, cuda_id=0, warm_up=1)
            if ret != 0:
                print(f"[错误] 初始化失败: {segmenter.get_last_error()}")
                cap.release()
                return
            print("[成功] 初始化成功！按 'q' 键退出。")
            time.sleep(2)

            while True:
                ret, frame = cap.read()
                if not ret:
                    break

                count, boxes, rendered_img, mask_img = segmenter.detect(img_np=frame, max_boxes=100)

                if count >= 0 and rendered_img is not None:
                    cv2.putText(rendered_img, f"Targets: {count}", (30, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)
                    cv2.imshow("DlSeg Camera Realtime Demo", rendered_img)
                else:
                    cv2.imshow("DlSeg Camera Realtime Demo", frame)

                if cv2.waitKey(1) & 0xFF in (ord('q'), 27):
                    break

    except Exception as e:
        print(f"[异常] {e}")
    finally:
        cap.release()
        cv2.destroyAllWindows()
    print("=== Demo 退出 ===")

if __name__ == "__main__":
    main()