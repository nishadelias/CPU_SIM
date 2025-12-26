# Common Compilation Issues and Fixes

## Issue 1: Multiple main() functions
**Problem**: `cpusim.cpp` has its own `main()` function and shouldn't be compiled with the GUI.

**Fix**: Removed `cpusim.cpp` from `CMakeLists.txt` CPU_SOURCES.

## Issue 2: Missing Qt includes
**Problem**: Missing includes for Qt classes like `QTableWidgetItem`, `QBrush`, `QColor`, etc.

**Fix**: Added missing includes to all widget files.

## Issue 3: Qt Charts namespace
**Problem**: `QT_CHARTS_USE_NAMESPACE` macro may not work in all Qt6 versions.

**Fix**: Changed to forward declarations and explicit namespace usage.

## Issue 4: QOverload syntax
**Problem**: `QOverload<int>::of(&QSpinBox::valueChanged)` may need different syntax.

**Fix**: Used lambda function instead for better compatibility.

## Issue 5: Static memory hierarchy
**Problem**: Using static variables for memory hierarchy can cause issues with multiple instances.

**Fix**: Changed to instance variables with proper initialization and cleanup.

## Issue 6: Type mismatches
**Problem**: Comparing `int` with `uint64_t` without proper casting.

**Fix**: Added explicit casts where needed.

## Building

If you still encounter errors:

1. **Check Qt6 installation**:
   ```bash
   qmake6 --version
   # or
   cmake --find-package Qt6
   ```

2. **Set Qt6 path explicitly**:
   ```bash
   cmake -DCMAKE_PREFIX_PATH=/path/to/qt6 ..
   ```

3. **Check for missing MOC files**:
   - Ensure `CMAKE_AUTOMOC ON` is set (it is)
   - Files with `Q_OBJECT` macro need MOC processing

4. **Common missing includes**:
   - `QIODevice` for file operations
   - `QAbstractItemView` for table widgets
   - `QPainter` for charts
   - `QMap` for maps

5. **If using Qt5 instead of Qt6**:
   - Change `Qt6` to `Qt5` in CMakeLists.txt
   - Some API differences may need adjustment

