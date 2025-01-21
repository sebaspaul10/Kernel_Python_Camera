import cv2
import time
import os

# Path to the signal file created/managed by the kernel thread
SIGNAL_FILE = "/tmp/motion_device"

def main():
    # Open the USB camera (adjust index if necessary)
    camera = cv2.VideoCapture(0, cv2.CAP_V4L2)  # Force V4L2 backend on Linux
    if not camera.isOpened():
        print("Error: Could not access the USB camera.")
        return

    print("Waiting for signal from the kernel thread...")

    while True:
        # Check if the signal file exists
        if os.path.exists(SIGNAL_FILE):
            print("Signal received from kernel thread. Opening camera...")

            while True:
                # Capture a frame from the camera
                ret, frame = camera.read()
                if not ret:
                    print("Failed to capture image from camera.")
                    break

                # Display the captured frame
                cv2.imshow("USB Camera", frame)

                # Exit the loop if 'q' is pressed
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break

            print("Closing camera. Waiting for the next signal...")
            # Break out of the inner loop and continue waiting for the next signal

        # Small delay to avoid excessive CPU usage
        time.sleep(0.5)

    # Release the camera and close all OpenCV windows
    camera.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
