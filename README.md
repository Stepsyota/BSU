# BSU Projects

This repository is a unified collection of various projects from different courses at BSU.  
It contains work in **C++**, **Computer_architecture**, **Numerical Methods**, **Computer Networks**, and **AI**.  

> ⚠️ **Note:** This is a "mix-and-match" archive. Individual repositories were merged into this monorepo using `git subtree`, preserving all commit histories.

---

## Repository Structure

- `C-plus-plus/` –  practice tasks  
- `AI/` – projects using Python and scikit-learn  
- `Computer_networks/` – tasks and simulations using Cisco Packet Tracer  
- `NumericalMethods/` – exercises implemented in C++  
- `Computer_architecture/`– exercises using x86-64 Assembly and C++


## Subproject Folders

Click on a badge to open each subproject folder:

[![C++](https://img.shields.io/badge/C%2B%2B-Folder-blue?style=for-the-badge)](https://github.com/Stepsyota/BSU/tree/main/C-plus-plus)
[![AI](https://img.shields.io/badge/AI-Folder-orange?style=for-the-badge)](https://github.com/Stepsyota/BSU/tree/main/AI)
[![Computer Networks](https://img.shields.io/badge/Networks-Folder-green?style=for-the-badge)](https://github.com/Stepsyota/BSU/tree/main/Computer_networks)
[![Numerical Methods](https://img.shields.io/badge/Numerical%20Methods-Folder-red?style=for-the-badge)](https://github.com/Stepsyota/BSU/tree/main/NumericalMethods)
[![Assembly](https://img.shields.io/badge/Assembly-Folder-lightgrey?style=for-the-badge)](https://github.com/Stepsyota/BSU/tree/main/Computer_architecture)
---

## Cloning Specific Subprojects

You don't need to clone the entire repository if you only want one subproject. For example, to clone only the **AI** folder:

```bash
git clone --filter=blob:none --sparse https://github.com/Stepsyota/BSU.git
cd BSU
git sparse-checkout set AI
```
