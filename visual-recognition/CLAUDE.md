# visual-recognition

Image classifier (CNN) from scratch in Python.

## Architecture

- `visual_recognition/layers.py` -- Conv2D, MaxPool, Dense, ReLU, Softmax
- `visual_recognition/network.py` -- Network class, forward/backward
- `visual_recognition/optimizer.py` -- SGD, Adam
- `visual_recognition/data.py` -- Dataset loading (CIFAR-10, MNIST)
- `visual_recognition/__main__.py` -- CLI (train, classify)

## Dev

```bash
python -m pytest tests/
python -m visual_recognition train --data mnist --epochs 5
```

## Conventions

- Python 3.9+, NumPy only (no PyTorch/TensorFlow)
- All math operations use NumPy arrays
