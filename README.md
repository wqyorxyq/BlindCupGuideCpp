# Blind Cup Guide C++

这是一个 C++ 原型项目，用电脑摄像头观察桌面上的杯子、手和其他物体，并通过语音提示帮助盲人用户抓到杯子。

当前已搭好完整代码骨架，训练数据集和模型可以之后补上。没有模型时程序会自动使用模拟检测结果，方便先测试语音和左右提示流程。

## 功能

- 浏览器前端操作界面，默认地址 `http://127.0.0.1:8080/`
- 前端可开始/停止识别、选择摄像头、切换模拟检测、开关语音播报
- 前端显示实时画面、当前语音提示、识别框、识别结果表和运行状态
- 摄像头实时采集画面
- OpenCV DNN 加载 ONNX 目标检测模型
- 识别逻辑预留三类：`cup`、`hand`、`other`
- 抓到杯子时播报：`做对了！`
- 未抓到杯子时根据杯子相对手的位置播报：`往左边一点` 或 `往右边一点`
- Windows 下优先调用 SAPI 语音接口，其他环境回退到控制台输出

## 目录

```text
include/blind_cup_guide/   C++ 头文件
src/                       C++ 实现
models/labels.txt          模型类别名，后续与训练模型保持一致
models/cup_detector.onnx   后续训练导出的模型放这里
docs/training-notes.md     数据集和模型训练说明
```

## 依赖

- C++17 编译器
- CMake 3.20+
- OpenCV，需包含 `core`、`videoio`、`highgui`、`imgproc`、`dnn`
- Windows 语音播报需要 Windows SDK/SAPI；也可以用 `--console-tts` 只打印语音文本

如果使用 vcpkg，可参考：

```powershell
vcpkg install opencv4:x64-windows
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root>\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Debug
```

## 运行

默认启动浏览器前端：

```powershell
$env:PATH = "Z:\opencv\build\x64\vc16\bin;$env:PATH"
.\build\default\Debug\blind_cup_guide.exe --mock
```

浏览器会打开：

```text
http://127.0.0.1:8080/
```

如果只启动服务、不自动打开浏览器：

```powershell
$env:PATH = "Z:\opencv\build\x64\vc16\bin;$env:PATH"
.\build\default\Debug\blind_cup_guide.exe --mock --no-open-browser --port 8080
```

保留的命令行调试模式：

```powershell
$env:PATH = "Z:\opencv\build\x64\vc16\bin;$env:PATH"
.\build\default\Debug\blind_cup_guide.exe --cli --mock
```

常用参数：

```text
--web                  启动浏览器前端，默认
--cli                  使用原 OpenCV 调试窗口
--port <port>          前端端口，默认 8080
--no-open-browser      启动服务但不自动打开浏览器
--camera <index>       摄像头编号，默认 0
--mock                 使用模拟检测结果跑通流程
--console-tts          不调用语音接口，只在控制台打印提示
--no-display           不显示调试窗口
--model <path>         ONNX 模型路径
--labels <path>        标签文件路径
```

训练好模型后，把模型放到：

```text
models/cup_detector.onnx
```

然后直接运行程序，不加 `--mock` 即可使用真实模型。

## 后续待做

- 采集并标注 `cup`、`hand`、`other` 数据集
- 训练并导出 ONNX 模型
- 根据实际模型输出格式微调 `OpenCvDnnObjectDetector`
- 做真实摄像头场景测试，调整抓取判定阈值
