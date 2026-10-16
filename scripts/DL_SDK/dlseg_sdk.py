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
        ("use_cuda", c_int),
        ("cuda_id", c_int),
        ("warm_up", c_int)
    ]


class DlSegSDK:
    """DlSeg C-API Python 封装类库"""

    def __init__(self, lib_path: str = None):
        """
        初始化并加载动态库
        :param lib_path: 动态库文件的自定义路径，若不传则自动根据系统类型推导相对路径
        """
        curr_dir = os.path.dirname(os.path.abspath(__file__))

        if sys.platform.startswith("win"):
            # ================= Windows 平台适配 =================
            bin_dir = os.path.abspath(os.path.join(curr_dir, "../../build/SDK_Demos"))

            if hasattr(os, "add_dll_directory") and os.path.exists(bin_dir):
                os.add_dll_directory(bin_dir)

            os.environ["PATH"] = bin_dir + os.path.pathsep + os.environ.get("PATH", "")

            if lib_path is None:
                lib_name = os.path.join(bin_dir, "DlSegSDK.dll")
            else:
                lib_name = lib_path

        elif sys.platform.startswith("linux"):
            # ================= Ubuntu / Linux 平台适配 =================
            lib_dir = os.path.abspath(os.path.join(curr_dir, "../../build/SDK_Demos"))
            self._load_all_so_in_dir(lib_dir)

            if lib_path is None:
                lib_name = os.path.join(lib_dir, "libDlSegSDK.so")
            else:
                lib_name = lib_path

        else:
            raise OSError(f"暂不支持的操作系统: {sys.platform}")

        # 加载 C++ SDK 动态库
        try:
            abs_lib_path = os.path.abspath(lib_name)
            if not os.path.exists(abs_lib_path):
                raise FileNotFoundError(f"未找到指定的动态库文件: {abs_lib_path}")

            if sys.platform.startswith("linux"):
                self._dll = ctypes.CDLL(abs_lib_path, mode=ctypes.RTLD_GLOBAL)
            else:
                self._dll = ctypes.CDLL(abs_lib_path)

            print(f"成功加载动态库: {abs_lib_path}")
        except OSError as e:
            print(f"动态库加载失败，请检查路径及依赖项! 详细错误: {e}")
            sys.exit(1)

        self._handle = None
        self._bind_functions()

    def _load_all_so_in_dir(self, lib_dir: str):
        """Ubuntu Linux 专属：遍历加载指定目录下所有的 .so 文件"""
        if not os.path.exists(lib_dir):
            return

        so_files = [
            f for f in os.listdir(lib_dir)
            if (f.endswith(".so") or ".so." in f) and f != "libDlSegSDK.so"
        ]

        max_attempts = 3
        for attempt in range(max_attempts):
            progress = False
            for so_file in list(so_files):
                so_path = os.path.join(lib_dir, so_file)
                try:
                    ctypes.CDLL(so_path, mode=ctypes.RTLD_GLOBAL)
                    so_files.remove(so_file)
                    progress = True
                except OSError:
                    pass
            if not progress:
                break

    def _bind_functions(self):
        """绑定 C 导出接口的参数与返回值类型 (与 C-API 头文件严格对应)"""
        self._dll.DlSeg_Create.restype = c_void_p
        self._dll.DlSeg_Create.argtypes = []

        self._dll.DlSeg_Destroy.restype = None
        self._dll.DlSeg_Destroy.argtypes = [c_void_p]

        self._dll.DlSeg_Initialize.restype = c_int
        self._dll.DlSeg_Initialize.argtypes = [c_void_p, POINTER(DlSegConfig)]

        self._dll.DlSeg_Detect.restype = c_int
        self._dll.DlSeg_Detect.argtypes = [
            c_void_p,
            POINTER(c_ubyte),
            c_int,
            c_int,
            POINTER(DlSegBox),
            c_int,
            c_char_p
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
        """创建 SDK 实例句柄"""
        if not self._handle:
            self._handle = self._dll.DlSeg_Create()
            if not self._handle:
                raise RuntimeError("创建 DlSeg 实例句柄失败，返回 NULL 指针")

    def initialize(self, model_path: str, use_cuda: int = 1, cuda_id: int = 0, warm_up: int = 1) -> int:
        """初始化 SDK 模型配置"""
        config = DlSegConfig(
            model_path=model_path.encode('utf-8'),
            use_cuda=use_cuda,
            cuda_id=cuda_id,
            warm_up=warm_up
        )
        return self._dll.DlSeg_Initialize(self._handle, byref(config))

    def detect(self, img_np: np.ndarray, save_path: str = "", max_boxes: int = 50) -> tuple[int, list[DlSegBox]]:
        """
        执行推理检测，并支持自定义保存带旋转框及掩膜的结果图片
        :param img_np: OpenCV 读取的 BGR 图像 (numpy.ndarray)
        :param save_path: 自定义保存路径及文件名（若为空则仅推理不保存）
        :param max_boxes: 最大接收目标数量缓冲区大小
        :return: (检测到的目标数量, 目标框对象列表)
        """
        if img_np is None or img_np.size == 0:
            raise ValueError("输入图像为空或无效")

        height, width = img_np.shape[:2]
        img_contiguous = np.ascontiguousarray(img_np, dtype=np.uint8)
        img_data_ptr = img_contiguous.ctypes.data_as(POINTER(c_ubyte))

        box_array = (DlSegBox * max_boxes)()
        path_bytes = save_path.encode('utf-8') if save_path else None

        count = self._dll.DlSeg_Detect(
            self._handle,
            img_data_ptr,
            width,
            height,
            box_array,
            max_boxes,
            path_bytes
        )

        detected_boxes = []
        if count > 0:
            copy_count = min(count, max_boxes)
            for i in range(copy_count):
                detected_boxes.append(box_array[i])

        return count, detected_boxes
    
    def save_mask(self, img_np: np.ndarray, boxes: list[DlSegBox], save_path: str, file_name: str) -> int:
        """
        独立提取并保存识别结果的全部掩膜图片
        :param img_np: 原始图像 (numpy.ndarray)
        :param boxes: detect 返回的框列表
        :param save_path: 自定义保存目录路径
        :param file_name: 自定义文件名
        :return: 状态码 (0 表示成功)
        """
        if img_np is None or img_np.size == 0:
            raise ValueError("输入图像为空")

        height, width = img_np.shape[:2]
        img_contiguous = np.ascontiguousarray(img_np, dtype=np.uint8)
        img_data_ptr = img_contiguous.ctypes.data_as(POINTER(c_ubyte))

        box_count = len(boxes)
        box_array = (DlSegBox * box_count)(*boxes) if box_count > 0 else (DlSegBox * 1)()

        path_bytes = save_path.encode('utf-8') if save_path else b""
        name_bytes = file_name.encode('utf-8') if file_name else b""

        status = self._dll.DlSeg_SaveMask(
            self._handle,
            img_data_ptr,
            width,
            height,
            box_array,
            box_count,
            path_bytes,
            name_bytes
        )
        return status

    def get_last_error(self) -> str:
        """获取最后一次详细错误描述字符串"""
        buf = create_string_buffer(512)
        self._dll.DlSeg_GetLastError(self._handle, buf, 512)
        return buf.value.decode('utf-8', errors='ignore')

    def get_last_status(self) -> int:
        """获取最后一次调用的状态码"""
        return self._dll.DlSeg_GetLastStatus(self._handle)

    def status_to_string(self, status: int) -> str:
        """将状态码数值转为描述字符串"""
        res_ptr = self._dll.DlSeg_StatusToString(status)
        if res_ptr:
            return res_ptr.decode('utf-8', errors='ignore')
        return f"Unknown Status Code: {status}"

    def destroy(self):
        """释放 C++ 侧底层资源"""
        if self._handle:
            self._dll.DlSeg_Destroy(self._handle)
            self._handle = None

    def __enter__(self):
        """Python 上下文管理器入口"""
        self.create()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Python 上下文管理器出口 (自动销毁实例)"""
        self.destroy()