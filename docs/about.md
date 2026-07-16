@mainpage Project Overview & Developer Guide

# Purpose

This project contains RISC-V Model (RVM) interface headers that aims to expose a unified interface to RISC-V functional simulators to allow them to be used them for simulation, co-simulation and verification interchangeably.
### **If you want to use simulator as a library but easily switch between implementations**

Instead of working through differences between different RISC-V simulator APIs to add them to your project you can simply use RVM interface as a simulator. Later to switch between them (e.g. to switch from qemu to spike) you can just pass another **.so** library to your existing program without rewriting any of it

### **If you want to run co-simulation**
To run co-simulation of different RISC-V simulators or even between a golden model and RTL you can implement RVM via both of them, then inside the drive build several @ref rvm::State instances (e.g. one for spike and one for sail-riscv) and then execute test program step-by-step on both of them at the same time. This way allows you to easily compare step results as you communicate between simulators via exact same interface.

## Quick Navigation
- @ref implementation "How to make my simulator compatible with RVM interface"
- @ref user-tutorial "How to use RVM interface"
- @ref RVM.hpp "C++ API"

