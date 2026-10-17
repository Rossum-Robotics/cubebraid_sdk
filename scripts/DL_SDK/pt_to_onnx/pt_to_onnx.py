from ultralytics import YOLO

def main():
    # 加载 YOLOv8 中等尺寸实例分割模型
    model = YOLO("yolov8m-seg.pt")

    # 导出为 ONNX 格式
    success = model.export(
        format="onnx",      # 导出格式
        imgsz=640,          # 输入图像尺寸（通常为 640x640）
        dynamic=False,      # 是否使用动态输入尺寸（False 为静态尺寸，部署推理通常推荐固定尺寸以获得更好性能）
        half=False,         # True 表示导出为 FP16 半精度（可减小模型体积并提升 GPU 推理速度），False 为标准 FP32
        simplify=True       # 是否使用 onnx-simplifier 简化计算图（强烈建议设为 True）
    )

    if success:
        print("[成功] YOLOv8m-seg 模型成功导出为 ONNX 格式！")
    else:
        print("[错误] 模型导出失败。")

if __name__ == "__main__":
    main()