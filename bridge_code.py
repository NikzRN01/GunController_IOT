import requests
import pyautogui
import time
import json

ESP32_IP = "192.168.4.1"  # Default ESP32 AP IP
CENTER_JOY = 2048  # Center value for joystick (adjust if needed)
SENSITIVITY = 0.1

def map_joystick_to_keys(joy_x, joy_y):
    """Map joystick to WASD keys"""
    # Release all movement keys
    if abs(joy_x - CENTER_JOY) < 200 and abs(joy_y - CENTER_JOY) < 200:
        return None
    
    keys = []
    if joy_y < CENTER_JOY - 200:  # Forward
        keys.append('w')
    if joy_y > CENTER_JOY + 200:  # Backward
        keys.append('s')
    if joy_x < CENTER_JOY - 200:  # Left
        keys.append('a')
    if joy_x > CENTER_JOY + 200:  # Right
        keys.append('d')
    
    return keys

def main():
    print("Connecting to gun_controller...")
    print("Make sure you're connected to 'gun_controller' WiFi")
    
    last_trigger = 0
    last_reload = 0
    
    while True:
        try:
            response = requests.get(f"http://{ESP32_IP}/", timeout=1)
            data = json.loads(response.text)
            
            # Handle joystick movement
            keys = map_joystick_to_keys(data['joyX'], data['joyY'])
            if keys:
                for key in keys:
                    pyautogui.keyDown(key)
            else:
                for key in ['w', 'a', 's', 'd']:
                    pyautogui.keyUp(key)
            
            # Handle gyroscope for mouse aiming
            mouse_x = data['gx'] * SENSITIVITY
            mouse_y = data['gy'] * SENSITIVITY
            pyautogui.moveRel(mouse_x, mouse_y)
            
            # Handle trigger
            if data['trigger'] == 1 and last_trigger == 0:
                pyautogui.click()
            last_trigger = data['trigger']
            
            # Handle reload
            if data['reload'] == 1 and last_reload == 0:
                pyautogui.press('r')
            last_reload = data['reload']
            
            print(f"Ammo: {data['ammo']}, Joy: ({data['joyX']}, {data['joyY']})")
            
        except Exception as e:
            print(f"Error: {e}")
            time.sleep(0.1)

if __name__ == "__main__":
    main()
