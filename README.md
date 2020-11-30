# QTask

![](https://github.com/jubnzv/qtask/workflows/Build/badge.svg)
[![License: GPL-3.0](https://img.shields.io/badge/License-GPL3-blue.svg)](https://opensource.org/licenses/GPL-3.0)

QTask is an open-source Qt-based graphical user interface for managing tasks. It is based on [Taskwarrior](https://taskwarrior.org/), a popular command-line organizer.

## Features

![image](assets/screenshot.png)

The goal of this application is to allow users to manage task list quickly using mostly the keyboards shortcuts while still having a user-friendly graphical user interface.

You may find the following features of this utility useful:

* Convenient GUI for adding, deleting, and editing tasks;
* Filters to quickly sort tasks based on Taskwarrior commands;
* Keyboard shortcuts for all common actions;
* Access to Taskwarrior CLI commands via the built-in shell;
* This utility monitors changes in the database in the background. Therefore, you will always see new tasks as they arrive. This is useful if you are using the Taskwarrior CLI or scripts like [bugwarrior](https://github.com/ralphbean/bugwarrior) at the same time.

If you have any ideas on how to improve this utility, feel free to create an [issue](https://github.com/jubnzv/qtask/issues) or open a [PR](https://github.com/jubnzv/qtask/pulls).

## Building

First of all, you need to install the dependencies. You will need Qt at least version 5.14.

On Debian-based distributions you need to run the following command:

```bash
sudo apt-get install qt5-default qttools5-dev libqt5svg5-dev libx11-xcb-dev qtbase5-private-dev
```

Clone the repository with submodules:

```bash
git clone --recurse-submodules https://github.com/jubnzv/qtask.git qtask
cd qtask
```

Build QTask in the build directory:

```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

Then you can install the compiled binary:

```bash
sudo make install
```

## License

GPL-3.0
