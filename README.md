# ThumbTwister: Setup & User Guide

ThumbTwister is a lightweight background service that rotates your physical thumbstick inputs at the driver level, ensuring all your games and applications read the perfectly rotated inputs without detecting your physical hardware.

## Important Prerequisites
* **Windows Defender SmartScreen:** Because this is an independent project without a corporate publisher certificate, Windows will flag the installer. When you run it, you will see a "Windows protected your PC" screen. Click **More info**, then click **Run anyway**.
* **Administrative Privileges:** The installer and the application require Admin rights to configure the controller hiding drivers safely. 
* **System Reboot:** You **must** restart your PC after the installation finishes.

---

## Installation
1. Download `ThumbTwister_Installer.exe`.
2. Double-click the file to run it (Bypass the SmartScreen prompt as mentioned above).
3. Click **Yes** when Windows asks for permission.
4. The setup will silently install the required drivers (ViGEmBus and HidHide) in the background. 
5. When the installation is complete, **Restart your computer**.

---

## Controller Setup
Before using ThumbTwister for the first time, you must tell the system exactly which physical controller you want to hide from your games. **You only have to do this once.**

1. Plug in your physical controller.
2. Open your Windows Start Menu and search for **HidHide Configuration Client**. Open it.
3. Go to the **Devices** tab at the top.
4. Look at the list of connected gaming devices. Find your physical controller and **tick the box next to it**.
5. Look at the bottom of the window and ensure the red padlock icon is **Locked** (Enable device hiding).
6. You can now close the HidHide Configuration Client.

*(Note: ThumbTwister has already been automatically added to the allowed application list during installation, so it is the only app that can see through this cloak)*

---

##  Daily Use
1. Launch **ThumbTwister** from your Start Menu or Desktop shortcut. 
2. Click **Yes** on the permission prompt.
3. The visualizer will open, showing your live, rotated thumbstick inputs. Your physical controller is now hidden, and your PC is reading the virtual controller. 
4. **To minimize:** If you close the visualizer window, ThumbTwister will continue running silently in the background. 
5. **To exit:** Look at your Windows System Tray (the little icons near your clock in the bottom right). Right-click the ThumbTwister icon and select **Exit ThumbTwister**. 

---

## Troubleshooting

**Q: My game is reading double inputs!**
**A:** This means your game is seeing both the physical controller and the virtual controller. Ensure you have completed **Step 2** correctly. Open the HidHide client and double-check that the box next to your physical controller is ticked and the red padlock is locked. 

**Q: The visualizer isn't moving when I push my thumbsticks.**
**A:** This usually happens if you launch ThumbTwister before plugging in your controller, or if Windows is holding onto an old hardware state. Exit ThumbTwister via the system tray, ensure your controller is plugged in and turned on, and launch ThumbTwister again.