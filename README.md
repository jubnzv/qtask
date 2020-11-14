# JTask

JTask is an open-source organizer based on [Taskwarrior](https://taskwarrior.org/).

## Screenshot

![image](assets/screenshot.png)

## Features

* GUI for adding, deleting, and editing Taskwarrior tasks
* Keyboard shortcuts for all actions
* Access to Taskwarrior CLI commands via the built-in shell
* The utility monitors updates in the Taskwarrior database and automatically updates the task list

## Building

On Debian-based distributions you need to install the following packages: `qt5-default qttools5-dev libqt5svg5-dev libx11-xcb-dev`.

To build, please do the following:

```cpp
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

## License

MIT
