# Models

ONNX model files are distributed separately from the code and from ONNX Runtime, and are never committed. Everything in this folder except this README is gitignored.

## Lane classifier contract

`conductor::ml::OnnxLaneClassifier` loads any `.onnx` file that matches this contract:

| | Name | Type | Shape |
|---|---|---|---|
| Input | `text` | string | `[1]` (the raw task brief) |
| Output | `scores` | float | `[1, 3]` in lane order: research, coding, general |

Tokenization and featurization live inside the model, for example a scikit-learn pipeline exported with skl2onnx, so the C++ side stays model-agnostic. The model must target an opset that ONNX Runtime 1.31 supports.

A prediction is only a suggestion. The deterministic router and policy still decide, and a low-confidence prediction leaves the task in the Inbox for a human.
