# augmented-reality

ARKit-based AR app in Swift.

## Architecture

- `Sources/App.swift` -- App entry point
- `Sources/ARViewContainer.swift` -- ARView wrapped for SwiftUI
- `Sources/PlaneDetection.swift` -- Horizontal/vertical plane detection
- `Sources/ObjectPlacement.swift` -- 3D object placement on surfaces
- `Sources/GestureHandler.swift` -- Tap, drag, pinch gestures

## Build

```bash
xcodegen generate
xcodebuild -scheme augmented-reality -destination 'platform=iOS Simulator,name=iPhone 17 Pro'
```

## Conventions

- SwiftUI + RealityKit
- iOS 17+, @Observable
- xcodegen for project generation
