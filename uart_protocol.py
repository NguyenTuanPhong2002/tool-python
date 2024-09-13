import serial
import time
import random

result = 0

def checksum(arr):
    return sum(arr)

def generate_random_array(n):
    return [random.randint(0, 255) for _ in range(n)]

def array_to_hex_string(arr):
    return ''.join(format(num, '02X') for num in arr)

def convert_to_bytes(number):
    high_byte = (number >> 8) & 0xFF
    low_byte = number & 0xFF
    return high_byte, low_byte

try:
    ser = serial.Serial('COM5', 115200, timeout=1)
    if ser.is_open:
        print(f"Cổng {ser.port} đã mở thành công!")
        
        for i in range(1000):
            random_number = random.randint(1, 150)
            # random_number = 80
            print("length ",random_number)
            
            random_array = generate_random_array(random_number)
            print("data ",random_array)
            
            checksum(random_array)
            # print(checksum(random_array))
            high, low = convert_to_bytes(checksum(random_array))
            # print(f"Byte cao: {high}, Byte thấp: {low}")
            
            # result_string = array_to_hex_string(random_array)
            # print("Chuỗi:", result_string)
            
            hex_string = f"FF0100{random_number:02X}{array_to_hex_string(random_array)}{high:02X}{low:02X}00"
            # print(hex_string)
            print(f"Lần {i} :Truyền chuỗi {hex_string}")
            data_to_send = bytes.fromhex(hex_string)
            start_time = time.time()
            ser.write(data_to_send)
            while True:
                if ser.in_waiting > 0:
                    data = ser.read()
                    if data:
                        if data.hex() == "50":
                            result += 1
                            print("Nhận thành công")
                            break
                        break
                if time.time() - start_time > 0.4:
                    print("Thời gian chờ hết, không có phản hồi.")
                    break
            
            # time.sleep(0.1)
            
        print(f"Số gói tin truyền nhận thành công {result}")

except serial.SerialException as e:
    print(f"Lỗi mở cổng: {e}")
finally:
    if ser.is_open:
        ser.close()
        print(f"Cổng {ser.port} đã được đóng.")
