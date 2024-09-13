import serial

try:
    ser = serial.Serial('COM4', 115200, timeout=1)
    if ser.is_open:
        print(f"Cổng {ser.port} đã mở thành công!")

        while True:
            data = ser.read()
            if data:
                # Giải mã dữ liệu sử dụng latin-1
                print(f"Dữ liệu nhận được (hex): {data.hex()}")

except serial.SerialException as e:
    print(f"Lỗi mở cổng: {e}")
finally:
    if ser.is_open:
        ser.close()
        print(f"Cổng {ser.port} đã được đóng.")
