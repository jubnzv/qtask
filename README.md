# QTask

![](https://github.com/jubnzv/qtask/workflows/Build/badge.svg)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

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

## Installation

Arch Linux users could use AUR to install `qtask`:
```bash
yay -S qtask-git
```

On other distributions you'll need to build it from sources.

### Building from source

First of all, you need to install the dependencies. You will need Qt at least version 5.14. For earlier versions of Qt, some features will be disabled.

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
