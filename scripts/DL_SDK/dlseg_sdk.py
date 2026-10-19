import os
import sys
import ctypes
import numpy as np

from ctypes import (
    Structure, c_void_p, c_int, c_float, c_ubyte, c_char_p,
    POINTER, CDLL, create_string_buffer, byref
)

class DlSegBox(Structure):
    """目标检测框及掩膜基础信息结构"""
    _fields_ = [
        ("id", c_int),
        ("confidence", c_float),
        ("x", c_int),
        ("y", c_int),
        ("width", c_int),
        ("height", c_int)
    ]

    def __repr__(self):
        return f"DlSegBox(id={self.id}, conf={self.confidence:.3f}, x={self.x}, y={self.y}, w={self.width}, h={self.height})"


class DlSegConfig(Structure):
    """深度学习分割模型配置结构"""
    _fields_ = [
        ("model_path", c_char_p),
        ("class_names_path", c_char_p),  # 类别名称文件路径
        ("use_cuda", c_int),
        ("cuda_id", c_int),
        ("warm_up", c_int),
        # 与 C++ 结构体对齐的配置字段
        ("net_width", c_int),
        ("net_height", c_int),
        ("class_threshold", c_float),
        ("nms_threshold", c_float),
        ("mask_threshold", c_float),
        ("batch_size", c_int)
    ]


class DlSegSDK:
    """DlSeg C-API Python 封装类库"""

    def __init__(self, lib_path: str = None):
        curr_dir = os.path.dirname(os.path.abspath(__file__))

        if sys.platform.startswith("win"):
            bin_dir = os.path.abspath(os.path.join(curr_dir, "../../build/SDK_Demos"))
            if hasattr(os, "add_dll_directory") and os.path.exists(bin_dir):
                os.add_dll_directory(bin_dir)
            os.environ["PATH"] = bin_dir + os.path.pathsep + os.environ.get("PATH", "")
            lib_name = os.path.join(bin_dir, "DlSegSDK.dll") if lib_path is None else lib_path
        elif sys.platform.startswith("linux"):
            lib_dir = os.path.abspath(os.path.join(curr_dir, "../../build/SDK_Demos"))
            self._load_all_so_in_dir(lib_dir)
            lib_name = os.path.join(lib_dir, "libDlSegSDK.so") if lib_path is None else lib_path
        else:
            raise OSError(f"暂不支持的操作系统: {sys.platform}")

        try:
            abs_lib_path = os.path.abspath(lib_name)
            if not os.path.exists(abs_lib_path):
                raise FileNotFoundError(f"未找到指定的动态库文件: {abs_lib_path}")

            if sys.platform.startswith("linux"):
                self._dll = ctypes.CDLL(abs_lib_path, mode=ctypes.RTLD_GLOBAL)
            else:
                self._dll = ctypes.CDLL(abs_lib_path)
        except OSError as e:
            print(f"动态库加载失败: {e}")
            sys.exit(1)

        self._handle = None
        self._bind_functions()

    def _load_all_so_in_dir(self, lib_dir: str):
        if not os.path.exists(lib_dir):
            return
        so_files = [f for f in os.listdir(lib_dir) if (f.endswith(".so") or ".so." in f) and f != "libDlSegSDK.so"]
        for _ in range(3):
            progress = False
            for so_file in list(so_files):
                try:
                    ctypes.CDLL(os.path.join(lib_dir, so_file), mode=ctypes.RTLD_GLOBAL)
                    so_files.remove(so_file)
                    progress = True
                except OSError:
                    pass
            if not progress:
                break

    def _bind_functions(self):
        self._dll.DlSeg_Create.restype = c_void_p
        self._dll.DlSeg_Create.argtypes = []

        self._dll.DlSeg_Destroy.restype = None
        self._dll.DlSeg_Destroy.argtypes = [c_void_p]

        self._dll.DlSeg_Initialize.restype = c_int
        self._dll.DlSeg_Initialize.argtypes = [c_void_p, POINTER(DlSegConfig)]

        # 绑定同时接收 rendered_img 和 mask_img 的 C 接口
        self._dll.DlSeg_Detect.restype = c_int
        self._dll.DlSeg_Detect.argtypes = [
            c_void_p,
            POINTER(c_ubyte),
            c_int,
            c_int,
            POINTER(DlSegBox),
            c_int,
            POINTER(c_ubyte),
            POINTER(c_ubyte)
        ]

        self._dll.DlSeg_GetLastError.restype = c_int
        self._dll.DlSeg_GetLastError.argtypes = [c_void_p, c_char_p, c_int]

        self._dll.DlSeg_GetLastStatus.restype = c_int
        self._dll.DlSeg_GetLastStatus.argtypes = [c_void_p]

        self._dll.DlSeg_StatusToString.restype = c_char_p
        self._dll.DlSeg_StatusToString.argtypes = [c_int]
        
        self._dll.DlSeg_SaveMask.restype = c_int
        self._dll.DlSeg_SaveMask.argtypes = [
            c_void_p,
            POINTER(c_ubyte),
            c_int,
            c_int,
            POINTER(DlSegBox),
            c_int,
            c_char_p,
            c_char_p
        ]

    def create(self):
        if not self._handle:
            self._handle = self._dll.DlSeg_Create()
            if not self._handle:
                raise RuntimeError("创建 DlSeg 实例句柄失败")

    def initialize(self, model_path: str, class_names_path: str = "", use_cuda: int = 1, cuda_id: int = 0, warm_up: int = 1,
                   net_width: int = 640, net_height: int = 640, class_threshold: float = 0.25, 
                   nms_threshold: float = 0.45, mask_threshold: float = 0.5, batch_size: int = 1) -> int:
        """初始化 SDK 模型，支持自定义输入尺寸与阈值"""
        self._model_path_bytes = model_path.encode('utf-8') if model_path else None
        self._class_names_path_bytes = class_names_path.encode('utf-8') if class_names_path else None

        config = DlSegConfig(
            model_path=self._model_path_bytes,
            class_names_path=self._class_names_path_bytes,
            use_cuda=use_cuda,
            cuda_id=cuda_id,
            warm_up=warm_up,
            net_width=net_width,
            net_height=net_height,
            class_threshold=class_threshold,
            nms_threshold=nms_threshold,
            mask_threshold=mask_threshold,
            batch_size=batch_size
        )
        return self._dll.DlSeg_Initialize(self._handle, byref(config))

    def detect(self, img_np: np.ndarray, max_boxes: int = 50) -> tuple[int, list[DlSegBox], np.ndarray, np.ndarray]:
        """
        执行推理检测，并同时输出渲染画布与纯净全局掩膜
        :return: (检测目标数量, 目标框列表, 渲染画布 numpy 数组, 单通道纯净掩膜 numpy 数组)
        """
        if img_np is None or img_np.size == 0:
            raise ValueError("输入图像为空")

        height, width = img_np.shape[:2]
        img_contiguous = np.ascontiguousarray(img_np, dtype=np.uint8)
        img_data_ptr = img_contiguous.ctypes.data_as(POINTER(c_ubyte))

        box_array = (DlSegBox * max_boxes)()
        rendered_buf = (c_ubyte * (width * height * 3))()
        mask_buf = (c_ubyte * (width * height))()

        count = self._dll.DlSeg_Detect(
            self._handle,
            img_data_ptr,
            width,
            height,
            box_array,
            max_boxes,
            rendered_buf,
            mask_buf
        )

        detected_boxes = []
        if count > 0:
            copy_count = min(count, max_boxes)
            for i in range(copy_count):
                detected_boxes.append(box_array[i])

        rendered_img = np.frombuffer(rendered_buf, dtype=np.uint8).reshape((height, width, 3)).copy()
        mask_img = np.frombuffer(mask_buf, dtype=np.uint8).reshape((height, width)).copy()

        return count, detected_boxes, rendered_img, mask_img
    
    def save_mask(self, img_np: np.ndarray, boxes: list[DlSegBox], save_path: str, file_name: str) -> int:
        if img_np is None or img_np.size == 0:
            raise ValueError("输入图像为空")

        height, width = img_np.shape[:2]
        img_contiguous = np.ascontiguousarray(img_np, dtype=np.uint8)
        img_data_ptr = img_contiguous.ctypes.data_as(POINTER(c_ubyte))

        box_count = len(boxes)
        box_array = (DlSegBox * box_count)(*boxes) if box_count > 0 else (DlSegBox * 1)()

        return self._dll.DlSeg_SaveMask(
            self._handle,
            img_data_ptr,
            width,
            height,
            box_array,
            box_count,
            save_path.encode('utf-8') if save_path else b"",
            file_name.encode('utf-8') if file_name else b""
        )

    def get_last_error(self) -> str:
        buf = create_string_buffer(512)
        self._dll.DlSeg_GetLastError(self._handle, buf, 512)
        return buf.value.decode('utf-8', errors='ignore')

    def get_last_status(self) -> int:
        return self._dll.DlSeg_GetLastStatus(self._handle)

    def destroy(self):
        if self._handle:
            self._dll.DlSeg_Destroy(self._handle)
            self._handle = None

    def __enter__(self):
        self.create()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.destroy()