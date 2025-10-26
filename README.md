# baidoxe
# Parking Lot

Dự án **Parking Lot** là hệ thống điều khiển bãi đỗ xe nhúng với vi điều khiển STM32 (hoặc tương tự), sử dụng các module như cảm biến, LCD, LED RGB, RFID, servo,... để quản lý trạng thái bãi đỗ xe.

## Mục lục

- [Giới thiệu](#giới-thiệu)  
- [Tính năng](#tính-năng)  
- [Yêu cầu phần cứng](#yêu-cầu-phần-cứng)  
- [Cấu trúc thư mục](#cấu-trúc-thư-mục)  
- [Cài đặt & Build](#cài-đặt--build)  
- [Sử dụng](#sử-dụng)  
- [Đóng góp](#đóng-góp)  
- [Bản quyền](#bản-quyền)

## Giới thiệu

Hệ thống này mô phỏng / điều khiển bãi đỗ xe:

- Quản lý trạng thái vị trí đỗ (có / không có xe)  
- Hiển thị thông tin lên màn hình LCD  
- Điều khiển LED RGB để báo trạng thái (ví dụ “đầy”, “còn trống”, “cảnh báo”)  
- Quay servo nếu cần cơ cấu vật lý như barie  
- Xác thực / checkin bằng RFID  

Tùy mục đích: bạn có thể dùng dự án này để làm bài tập môn nhúng, prototype hệ thống bãi đỗ xe tự động, hoặc demo IoT.

## Tính năng

- Khởi tạo hệ thống (cấu hình GPIO, timer, ngoại vi)  
- Đọc trạng thái từ cảm biến / mạch GPIO  
- Điều khiển LED RGB và LED 7-segment  
- Điều khiển servo (barie) để đóng / mở bãi đỗ  
- Sử dụng RFID để kiểm soát xe ra / vào  
- Hiển thị thông tin lên LCD (có thể là số xe, trạng thái bãi, thông báo lỗi)

## Yêu cầu phần cứng

- Vi điều khiển STM32 (hoặc MCU tương tự)  
- Module LCD (tùy model)  
- LED RGB  
- LED 7-segment  
- Servo  
- Module RFID  
- Nguồn + dây kết nối  

Ngoài ra: dụng cụ nạp (ST-Link, SWD, v.v.), PC để biên dịch.

## Cấu trúc thư mục

