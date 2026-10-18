from ultralytics import YOLO

def main():
    # 加载实例分割模型
    model = YOLO("yolo11m-seg.pt")

    # 导出模型
    model.export(format="onnx")

if __name__ == "__main__":
    main()