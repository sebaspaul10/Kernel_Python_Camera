import cv2
import time
import os

DEVICE_FILE = "/dev/motion_device"

def main():
    # Open the USB camera
    camera = cv2.VideoCapture(0, cv2.CAP_V4L2)
    if not camera.isOpened():
        print("Error: Could not access the USB camera.")
        return

    print("Waiting for motion signal from the kernel module...")

    while True:
        try:
            with open(DEVICE_FILE, 'r') as device:
                motion_status = device.read().strip()
                if "Motion detected" in motion_status:
                    print("Motion detected! Opening camera...")

                    while True:
                        ret, frame = camera.read()
                        if not ret:
                            print("Failed to capture image from camera.")
                            break

                        cv2.imshow("USB Camera", frame)

                        if cv2.waitKey(1) & 0xFF == ord('q'):
                            break

                    print("Closing camera. Waiting for the next signal...")

        except FileNotFoundError:
            print(f"Error: Device file {DEVICE_FILE} not found.")
            break

        time.sleep(0.5)

    camera.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()

