# Training Notes

This repository includes a trained ONNX object detector. To replace or retrain it, prepare your own dataset; the original images and training checkpoints are not included. The runtime expects three logical classes:

1. `cup`
2. `hand`
3. `other`

Recommended workflow:

1. Capture images from the same camera angle and desk height that the final demo will use.
2. Label bounding boxes for the cup, the user's hand, and the non-cup object.
3. Train an object detector such as YOLOv8/YOLOv11, RT-DETR, or another ONNX-exportable model.
4. Export the model to ONNX and place it at `models/cup_detector.onnx`.
5. Keep `models/labels.txt` in the same class order as the exported model.
6. Run the app without `--mock` to use the trained model.

Guidance logic:

- If the hand overlaps the cup, the app says `做对了！`.
- If the hand is away from the cup, the app compares the hand center with the cup center.
- If the cup is left of the hand, the app says `往左边一点`.
- If the cup is right of the hand, the app says `往右边一点`.

For a real assistive workflow, collect data from different lighting conditions, cup colors, hand positions, and desk backgrounds. Test with the target user before relying on the guidance.

## Recorded Training Run

The metrics below are historical experiment records, not a fresh evaluation. All example paths are relative to the repository root. Prepare `datasets/cup_guide/data.yaml` and the corresponding images and labels before running training; update dataset paths in that YAML to match your data layout.

Training is optional: the C++ application can use the included ONNX file without Python, CUDA or Ultralytics. To reproduce the recorded training environment, prepare a separate Python environment with compatible Ultralytics and PyTorch versions. `device=0` in the example requires a supported GPU; use `device=cpu` when needed.

Dataset:

- `datasets/cup_guide`
- Train images: 203
- Validation images: 25
- Test images: 26
- Classes: `cup`, `hand`, `other`

Environment:

- Python 3.12.9
- Ultralytics 8.4.61
- PyTorch 2.11.0+cu128
- GPU: NVIDIA GeForce RTX 4060 Laptop GPU

Training command:

```powershell
yolo detect train data=datasets/cup_guide/data.yaml model=yolo11n.pt epochs=100 imgsz=640 batch=8 device=0 project=runs name=cup_guide_yolo11n exist_ok=True workers=0 patience=25 cache=False plots=True
```

Best checkpoint:

```text
runs/cup_guide_yolo11n/weights/best.pt
```

Validation result for the best checkpoint:

```text
all    mAP50=0.753  mAP50-95=0.627
cup    mAP50=0.864  mAP50-95=0.766
hand   mAP50=0.766  mAP50-95=0.526
other  mAP50=0.630  mAP50-95=0.589
```

ONNX export command:

```powershell
yolo export model=runs/cup_guide_yolo11n/weights/best.pt format=onnx imgsz=640 simplify=True opset=12
```

Copy the exported `runs/cup_guide_yolo11n/weights/best.onnx` file to:

```text
models/cup_detector.onnx
```

ONNX validation result:

```text
all    mAP50=0.734  mAP50-95=0.606
cup    mAP50=0.870  mAP50-95=0.753
hand   mAP50=0.793  mAP50-95=0.559
other  mAP50=0.537  mAP50-95=0.506
```

Next data improvements:

- Add more `other` examples such as phones, mice, bottles, keys, books, bowls, and boxes.
- Add more hard negatives: images with no cup, only hand, only other objects, and partially visible cups.
- Add more lighting variation and desk backgrounds.
- Keep the final camera angle close to the angle used during data collection.
