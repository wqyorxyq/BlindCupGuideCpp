# Training Notes

This project is ready for a future ONNX object detector. The runtime expects three logical classes:

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
