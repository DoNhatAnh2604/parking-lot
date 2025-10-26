
# 🚗 HỆ THỐNG QUẢN LÝ NHÀ XE TỰ ĐỘNG (Parking Lot Project)

## 👥 Thành viên thực hiện

| Họ và tên | Phụ trách |
|------------|------------|
| **Huy** | Code main, Cảm biến IR, RFID, LED 7 đoạn, Quay video demo |
| **Nhất Anh** | Làm mô hình, LCD, LED RGB, Viết README, hoàn thiện báo cáo |

---

## 🧩 Giới thiệu

Dự án **Nhà xe tự động (Parking Lot)** là hệ thống mô phỏng quản lý bãi đỗ xe thông minh, được thực hiện bằng **vi điều khiển STM32** và các module ngoại vi thông dụng như **RFID, cảm biến hồng ngoại (IR), LCD, LED RGB, LED 7 đoạn, Servo**, v.v.

Mục tiêu của hệ thống là giúp **tự động hóa việc ra vào và quản lý chỗ trống** trong nhà xe, đồng thời cung cấp **thông tin trực quan** cho người dùng và người quản lý.

---

## 💡 Chức năng tổng quát

### 🔶 1. Đèn báo hiệu tình trạng bãi xe

- **Xanh lá:** còn **3–4 chỗ trống**  
- **Vàng:** còn **1–2 chỗ trống**  
- **Đỏ:** **đầy (0 chỗ trống)**  

💡 Khi người lái xe đi gần đến khu gửi xe, họ có thể **quan sát đèn thông báo** để biết trước tình trạng bãi xe mà không cần phải đi vào.

---

### 🅿️ 2. Cổng ra/vào & Màn hình LCD

- **LCD** hiển thị:
  - Số lượng **xe đang có trong bãi**  
  - Số lượng **chỗ trống còn lại**  
  - Trạng thái **mở / đóng cửa**  
  - **Thời gian đóng cửa** nếu bãi xe đầy  

🔐 Khi bãi xe đã đầy, **hệ thống sẽ tự động đóng cửa và hiển thị thông báo “Đầy chỗ”**.

---

### 🔁 3. Cảm biến IR và LED 7 đoạn

- Có **2 cảm biến IR** được đặt tại cổng ra/vào:
  - Khi xe **đi vào**, **LED 7 đoạn tăng** giá trị hiển thị.
  - Khi xe **đi ra**, **LED 7 đoạn giảm** giá trị hiển thị.
- Giá trị hiển thị trên **LED 7 đoạn** thể hiện **tổng số xe đang có trong nhà xe**, giúp **bảo vệ** theo dõi dễ dàng hơn.

---

### 🪪 4. Quét thẻ RFID

- Mỗi xe được cấp **thẻ RFID** riêng.  
- Xe **chỉ được ra/vào** khi quét **đúng thẻ hợp lệ**.  
- Hệ thống giúp **kiểm soát an ninh** và **ngăn chặn ra vào trái phép**.

---

## 🚀 Hướng phát triển thêm

1. **Đồng hồ thời gian thực (RTC):**  
   - Ghi lại **thời gian vào / ra** của từng xe để tra cứu khi cần thiết.  
   - Hỗ trợ **thống kê thời gian gửi xe** hoặc xử lý các trường hợp bất thường.

2. **Định vị vị trí chỗ đỗ xe:**  
   - Gợi ý **chỗ trống khả dụng** cho người dùng (đặc biệt khi bãi có ít chỗ).  
   - Có thể mở rộng lên **mô hình hiển thị bản đồ bãi xe nhỏ**.

---

## ⚙️ Thành phần phần cứng chính

- Vi điều khiển **STM32F103C8T6 (Bluepill)**  
- Module **RFID RC522**  
- Cảm biến **IR (Infrared Sensor)**  
- Màn hình **LCD 16x2 / 20x4**  
- **LED RGB** (báo hiệu tình trạng bãi xe)  
- **LED 7 đoạn** (hiển thị số xe hiện có)  
- **Servo** (điều khiển barie cổng)  





