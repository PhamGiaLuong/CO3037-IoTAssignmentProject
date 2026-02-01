# IoT Assignment Project: RTOS-based Embedded System on ESP32-S3

## 1. Giới thiệu tổng quan (Project Overview)

Dự án này là bài tập lớn môn học Phát triển ứng dụng IoT (CO3037), tập trung phát triển hệ thống nhúng thời gian thực (Real-time embedded systems) trên nền tảng ESP32-S3 sử dụng PlatformIO.

Mục tiêu của dự án là xây dựng một hệ thống hoàn chỉnh dựa trên RTOS, mở rộng và thay đổi ít nhất 30% so với mã nguồn gốc, bao gồm các tính năng giám sát cảm biến, hiển thị LCD, Web Server và tích hợp TinyML.

### Danh sách các Task (Project Tasks)

Dưới đây là các nhiệm vụ chính cần thực hiện. Các thành viên cập nhật chi tiết tính năng vào phần "Description" khi triển khai.

#### Task 1: Single LED Blink with Temperature Conditions 
* **Mô tả:** Điều khiển hành vi nháy LED dựa trên điều kiện nhiệt độ.
* **Chi tiết tính năng:**
    * [TODO: Mô tả logic nháy LED 1]
    * [TODO: Mô tả logic nháy LED 2]
    * [TODO: Cơ chế Semaphore sử dụng]

#### Task 2: NeoPixel LED Control Based on Humidity
* **Mô tả:** Điều khiển màu sắc LED NeoPixel dựa trên độ ẩm.
* **Chi tiết tính năng:**
    * [TODO: Mapping dải độ ẩm - màu sắc]
    * [TODO: Logic đồng bộ hóa Semaphore]

#### Task 3: Temperature and Humidity Monitoring with LCD
* **Mô tả:** Hiển thị thông tin lên LCD, quản lý trạng thái hiển thị (Normal/Warning/Critical) và loại bỏ biến toàn cục.
* **Chi tiết tính năng:**
    * [TODO: Các trạng thái hiển thị]
    * [TODO: Phương pháp loại bỏ global variables]

#### Task 4: Web Server in Access Point Mode
* **Mô tả:** Xây dựng Web Server điều khiển thiết bị, giao diện người dùng cải tiến.
* **Chi tiết tính năng:**
    * [TODO: Thiết kế giao diện]
    * [TODO: Chức năng điều khiển nút nhấn]

#### Task 5: TinyML Deployment & Accuracy Evaluation
* **Mô tả:** Triển khai mô hình TinyML nhận diện dữ liệu trên vi điều khiển.
* **Chi tiết tính năng:**
    * [TODO: Mô tả tập Dataset]
    * [TODO: Đánh giá độ chính xác trên phần cứng]

#### Task 6: Data Publishing to CoreIOT Cloud Server
* **Mô tả:** Đẩy dữ liệu cảm biến lên CoreIOT Server qua WiFi (Station Mode).
* **Chi tiết tính năng:**
    * [TODO: Cấu hình xác thực Token]
    * [TODO: Tần suất gửi dữ liệu]

---

## 2. Cấu trúc mã nguồn (Project Structure)

Dự án được xây dựng trên **PlatformIO**, cấu trúc thư mục được tổ chức như sau:

```text
.
├── boards/                # Chứa file định nghĩa board
├── include/               # Chứa các file header (.h)
├── lib/                   # Kho lưu trữ các thư viện bên NGOÀI (bản Local)
├── src/                   # Source code chính (.cpp)
│   └── main.cpp           # Entry point
├── .clang-format          # File quy tắc định dạng code (Google Style)
├── .gitignore             # Cấu hình file/folder cần bỏ qua trong git
├── platformio.ini         # Cấu hình project, thư viện, environment cho ESP32-S3
└── README.md              # Tài liệu hướng dẫn dự án
```

### 2.1. Quản lý thư viện bên ngoài (External Libraries)

Để đảm bảo tính ổn định và tránh lỗi do cập nhật phiên bản từ remote, **KHÔNG** sử dụng đường dẫn thư viện online trực tiếp.

* **Vị trí:** Tất cả thư viện bên ngoài phải được tải về và lưu trữ bản local trong thư mục `lib/`.
* **Quy tắc đặt tên:** Thư mục thư viện phải tuân thủ format: `[Tên-Gốc]-v[Phiên-bản]`.
* *_Ví dụ:_* `ArduinoHttpClient-v0.6.1`, `DHT-AdafruitLibrary-v1.4.6`.
* **Cấu hình:** Khai báo thư viện trong `platformio.ini` tại mục `lib_deps` để trình biên dịch nhận diện chính xác phiên bản local.

### 2.2. Tổ chức Source Code (Source & Header Separation)

Dự án tuân thủ mô hình tách biệt giữa khai báo (Interface) và triển khai (Implementation):

* **Mỗi file `.cpp` trong `src/`** (chứa code thực thi) phải có **một file `.h` tương ứng trong `include/`** (chứa khai báo hàm, struct, define).
* **Mục đích:** Giúp code rõ ràng, dễ quản lý dependency và tránh lỗi "multiple definition" khi include chéo.

### 2.3. Đồng bộ Format Code (Code Consistency)
Để đảm bảo mã nguồn thống nhất, dễ đọc giữa các thành viên:

* **Công cụ:** Dự án sử dụng **Clang-Format** với chuẩn **Google Style** (đã tùy chỉnh cho Embedded).
* **Cơ chế tự động:**
1. File `.clang-format` tại thư mục gốc chứa các quy tắc định dạng (indent 4 spaces, cách đặt ngoặc,...).
2. Thư mục `.vscode/settings.json` đã được cấu hình để kích hoạt **Format On Save**.

* **Yêu cầu:** Thành viên **KHÔNG** được thay đổi nội dung file `.clang-format` và `.vscode/settings.json`. Khi code xong, chỉ cần nhấn `Ctrl + S`, code sẽ tự động được format chuẩn trước khi commit.

---

## 3. Quy tắc làm việc chung

### 3.1. Quy tắc làm việc với Git

Để đảm bảo điểm số phần **Git Contribution**, nhóm tuân thủ các quy tắc sau:

#### a. Hệ thống phân nhánh (Branching Strategy):

* **`main`**: Nhánh chính, chỉ chứa code đã hoàn thiện, ổn định và sẵn sàng để demo.
* **`integration`**: Nhánh trung gian để các thành viên merge code, kiểm tra xung đột (conflict) trước khi đưa lên `main`.
* **`member-name-1`**, **`member-name-2`**, **`member-name-3`**: Nhánh riêng của từng thành viên để phát triển tính năng độc lập.

#### b. Quy trình Merge:

1. Hoàn thiện tính năng trên nhánh cá nhân.
2. Pull code mới nhất từ `integration` về để giải quyết conflict ở dưới local.
3. Tạo Pull Request (PR) để merge vào `integration`.

#### c. Commit Convention:

Sử dụng format: `[Loại] - Nội dung thay đổi ngắn gọn`, ngôn ngữ tiếng Anh.

* `feat`: Thêm tính năng mới.
* `fix`: Sửa lỗi.
* `docs`: Cập nhật tài liệu, comment.
* `refactor`: Tối ưu cấu trúc code (không thay đổi chức năng).
* _Ví dụ:_ `feat: Add NeoPixel control logic` hoặc `fix: Resolve semaphore deadlock in Task 1`

### 3.2. Quy tắc Code (Coding Standards)

Tuân thủ quy tắc để code thống nhất và dễ đọc:

#### a. Đặt tên (Naming Convention):

* **Biến (Variables):** Sử dụng **camelCase**
  * Ví dụ: `currentTemperature`, `isSystemReady`
* **Hàm (Functions):** Sử dụng **PascalCase**
  * Ví dụ: `ReadSensorData()`, `UpdateLcdDisplay()`
* **File:** Sử dụng **PascalCase** 
  * Ví dụ: `SensorModule.cpp`
* **Hằng số:** Viết HOA toàn bộ
  * Ví dụ: `MAX_HUMIDITY_THRESHOLD`

#### b. Comment & Tài liệu:

* **Header Comment:** Mỗi file cần có phần giới thiệu ngắn gọn ở đầu file (Tên file, Tác giả, Chức năng chính).
* **Logic Comment:** Bắt buộc comment giải thích các đoạn logic phức tạp để người khác dễ hiểu khi review.
* **Ngôn ngữ:** Sử dụng tiếng Anh.

---

## 4. Thông tin nhóm
* **Thành viên 1:** [Họ tên] - Vai trò:
* **Thành viên 2:** [Họ tên] - Vai trò:
* **Thành viên 3:** [Họ tên] - Vai trò:

