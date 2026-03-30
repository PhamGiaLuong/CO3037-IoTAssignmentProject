# IoT Assignment Project: RTOS-based Embedded System on ESP32-S3

## 1. Giới thiệu tổng quan (Project Overview)

Dự án này là bài tập lớn môn học Phát triển ứng dụng IoT (CO3037), tập trung phát triển hệ thống nhúng thời gian thực (Real-time embedded systems) trên nền tảng ESP32-S3 sử dụng PlatformIO.

Mục tiêu của dự án là xây dựng một hệ thống hoàn chỉnh dựa trên RTOS, mở rộng và thay đổi ít nhất 30% so với mã nguồn gốc, bao gồm các tính năng giám sát cảm biến, hiển thị LCD, Web Server và tích hợp TinyML.

### Danh sách các Task (Project Tasks)

Dưới đây là các nhiệm vụ chính cần thực hiện. Các thành viên cập nhật chi tiết tính năng vào phần "Description" khi triển khai.

#### Task 1: Single LED Blink with Temperature Conditions 
* **Mô tả:** Điều khiển hành vi nháy LED cảnh báo, được thiết kế tách biệt hoàn toàn thành 2 module để phục vụ cho kiến trúc Gateway và Sensor Node.
* **Chi tiết tính năng:**
    * **Logic nháy LED Sensor Node:** Cảnh báo các bất thường tại buồng lạnh. Trạng thái Normal (Nháy chậm: 500ms ON / 2500ms OFF). Trạng thái Warning (Nháy vừa: Cảnh báo nhiệt độ/độ ẩm vượt ngưỡng hoặc AI phát hiện cửa mở quá lâu). Trạng thái Critical (Nháy nhanh: Lỗi mất kết nối cảm biến DHT20 hoặc màn hình LCD).
    * **Logic nháy LED Gateway Node:** Cảnh báo tình trạng mạng trung tâm. Trạng thái Normal (Đã kết nối CoreIoT an toàn). Trạng thái Warning (Đang ở chế độ phát WiFi - AP Mode). Trạng thái Critical (Mất kết nối WiFi hoặc rớt mạng MQTT Cloud).
    * **Cơ chế Semaphore sử dụng:** Sử dụng 2 Binary Semaphore độc lập (sensor_led_sync_semaphore và gw_led_sync_semaphore). Thay vì Polling tốn CPU, các Task giám sát khi phát hiện lỗi sẽ xSemaphoreGive để đánh thức lập tức Task LED, ép nó đổi chu kỳ chớp tắt theo thời gian thực (Real-time).

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
* **Mô tả:** Xây dựng Web Server chạy trên ESP32 (Gateway) đóng vai trò là Captive Portal quản lý tập trung toàn bộ hệ thống qua giao diện SPA (Single Page Application) offline.
* **Chi tiết tính năng:**
    * **Dashboard:** Hiển thị trạng thái chung của Gateway và tạo các thẻ (Card) giám sát độc lập cho từng Sensor Node (Nhiệt độ, Độ ẩm, Peripheral, AI Prediction). Tích hợp biểu đồ lịch sử Dual Y-Axis (sử dụng Canvas HTML5 thuần không cần Internet) vẽ 30 điểm dữ liệu gần nhất của từng buồng lạnh.
    * **Điều khiển (Controls):** Giao diện điều khiển Relay thực tế (Siren Alarm & Exhaust Fan), nút ép buộc Gateway chuyển sang chế độ WiFi (Station Mode), và đặc biệt có thêm tính năng Node Remote Control để gửi lệnh Reset phần cứng từ xa xuống từng Sensor Node thông qua giao thức ESP-NOW.
    * **Cấu hình hệ thống (Settings):** Lưu trữ Flash cho cấu hình Mạng (AP, WiFi) và Cloud (MQTT CoreIoT). Đặc biệt, cung cấp giao diện Quản lý Node (Pair/Unpair bằng MAC Address) và cho phép cấu hình đồng bộ ngưỡng cảnh báo (Min/Max Temp/Hum) riêng biệt xuống từng Node con thông qua ESP-NOW.

#### Task 5: TinyML Deployment & Accuracy Evaluation
* **Mô tả:** Triển khai mô hình AI phân loại trạng thái buồng lạnh thành 3 nhãn: Bình thường (Normal), Cửa mở (Door Open) và Lỗi hệ thống (System Fault).
* **Chi tiết tính năng:**
    * **Tập Dataset & Kỹ thuật Feature Engineering:** Dữ liệu được thu thập trực tiếp từ ESP32 qua Serial bằng script Python. Thay vì học giá trị thô, mô hình được huấn luyện bằng 4 đặc trưng: Giá trị chuẩn hóa (t_norm, h_norm - giúp mô hình chạy chuẩn xác với mọi buồng lạnh mà không phụ thuộc vào ngưỡng cài đặt) và Tốc độ thay đổi (t_speed, h_speed - giúp phân biệt việc cửa mở đột ngột so với máy lạnh rò rỉ hư hỏng từ từ).
    * **Cơ chế Debounce trên phần cứng:** Kết hợp kết quả phân loại từ TFLite Micro với logic quản lý trạng thái trên C++. Khi AI báo "Door Open", hệ thống không báo động ngay mà bắt đầu đếm ngược (Debounce ~ 20 giây). Nếu cửa vẫn mở quá thời gian quy định, cờ Semaphore cảnh báo mới được kích hoạt. Quá trình Inference tốn chưa đến 1ms và cấp phát bộ nhớ Arena Size chỉ khoảng 8KB RAM.

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
├── data/                  # Chứa các file mã nguồn nạp vào phân vùng LittleFS cho Web Server
├── include/               # Chứa các file header (.h)
├── lib/                   # Kho lưu trữ các thư viện bên NGOÀI (bản Local)
├── src/                   # Source code chính (.cpp)
│   └── main.cpp           # Entry point
├── tinyml/                # Source thu thập/đánh nhãn dữ liệu, huấn luyện và export AI model
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
* **`dev/gia-luong`**, **`dev/quang-minh`**: Nhánh riêng của từng thành viên để phát triển tính năng độc lập.

#### b. Quy trình Merge:

1. Hoàn thiện tính năng trên nhánh cá nhân và push lên remote
   ```
   git push origin dev/<member-name>
   ```
2. Pull code mới nhất từ `integration` về để merge nhánh cá nhân vào và giải quyết conflict ở dưới local.
3. Sau đó push lên lại
   ```
   git push origin integration
   ```
4. Sau khi hoàn thiện tất cả các tính năng (đã test) mới merge vào `main` để nộp

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

* **snake_case:** Sử dụng cho **biến (variable)**, **File**
  * Ví dụ: `current_temperature`, `is_system_ready`, `global.cpp`, `web_server.h`
* **camelCase** Sử dụng **hàm (Function)**
  * Ví dụ: `readSensorData()`, `updateLcdDisplay()`
* **PascalCase** Sử dụng **Class/Struct**
  * Ví dụ: `SensorData`
* **Hằng số và Macro:** Viết HOA toàn bộ
  * Ví dụ: `MAX_HUMIDITY_THRESHOLD`

#### b. Comment & Tài liệu:

* **Logic Comment:** Bắt buộc comment giải thích các đoạn logic phức tạp để người khác dễ hiểu khi review.
* **Ngôn ngữ:** Sử dụng tiếng Anh.

#### c. Log trạng thái (Logging Standards)
* **Không sử dụng `Serial.print()` rải rác**: Việc gọi trực tiếp hàm in từ các task sẽ làm Serial Monitor khó đọc và lỗi in.
* **Sử dụng Macro Thread-Safe**: Bắt buộc sử dụng các Logging Macro đã được bọc Mutex an toàn định nghĩa sẵn trong `global.h` gồm: `LOG_INFO`, `LOG_WARN`, và `LOG_ERR`.
* **Định dạng thông báo**: Mọi dòng log phải tuân thủ chuẩn `[LOẠI] [TÊN_MODULE] Nội dung` (Ví dụ đúng: `LOG_INFO("SENSOR", "Read success. Temp: %.2f", temp);`).

```
[INFO] [SENSOR] Read success. Temp: 28.50
[INFO] [WIFI] Connected to IP: 192.168.1.15
[WARN] [SENSOR] High temperature detected: 36.20 C
[ERR]  [CLOUD] Failed to publish data to CoreIoT.
```  
---

## 4. Hướng dẫn Thu thập dữ liệu, Huấn luyện và Deploy Mô hình TinyML (MLOps)

Dự án áp dụng quy trình tự động hóa nhằm đơn giản hóa việc triển khai Machine Learning lên vi điều khiển. Mô hình phân loại 3 trạng thái: `0` (Normal - Bình thường), `1` (Door Open - Cửa mở), và `2` (System Fault - Lỗi hệ thống). Nhờ kỹ thuật Feature Engineering (Chuẩn hóa và Tốc độ thay đổi), mô hình độc lập với ngưỡng phần cứng (Threshold-Agnostic) và không cần train lại khi thay đổi buồng lạnh.

### Bước 1: Thiết lập môi trường (Environment Setup)
Khuyến nghị sử dụng Anaconda để quản lý môi trường ảo, tránh xung đột thư viện.

1. Mở Terminal / Anaconda Prompt và di chuyển vào thư mục chứa code ML:
   ```bash
   cd tinyml
   ```
2. Khởi tạo và kích hoạt môi trường ảo với Python 3.10:
   ```bash
   conda create --name ccms-ml-training python=3.10 -y
   conda activate ccms-ml-training
   ```
3. Cài đặt các thư viện cần thiết:
   ```bash
   pip install -r requirements.txt
   ```

### Bước 2: Thu thập dữ liệu (Data Collection)
Đảm bảo ESP32 đang được cắm vào máy tính và liên tục in ra Serial định dạng `Temp,Hum\n` (ví dụ: `5.5,65.2`). Sử dụng script `data_collector.py` để đọc từ cổng COM và tự động lưu vào file `sensor_data.csv`.

Chạy các lệnh sau tương ứng với từng kịch bản bạn muốn thu thập (thay `COM3` bằng cổng thực tế):

* **Thu thập dữ liệu Bình thường (Label 0):**
  ```bash
  python data_collector.py --port COM3 --label 0
  ```
* **Thu thập dữ liệu Giả lập Mở cửa (Label 1):** (Nhiệt độ/Độ ẩm tăng nhanh đột ngột)
  ```bash
  python data_collector.py --port COM3 --label 1
  ```
* **Thu thập dữ liệu Giả lập Lỗi hệ thống (Label 2):** (Nhiệt độ tăng từ từ, rỉ rả do máy nén hỏng)
  ```bash
  python data_collector.py --port COM3 --label 2
  ```
*(Nhấn `Ctrl+C` để dừng thu thập và lưu file. Script sẽ tự động ghi nối tiếp (append) vào dataset).*

### Bước 3: Huấn luyện và Export Mô hình (Model Training)
Script `train_model.py` sẽ tự động tính toán trích xuất đặc trưng (t_norm, h_norm, t_speed, h_speed), chia tập dữ liệu, huấn luyện mạng Neural Network và lượng tử hóa (Quantization) mô hình.

Chạy lệnh sau để bắt đầu huấn luyện:
```bash
python train_model.py
```
**Kết quả:** Sau khi quá trình hoàn tất, script sẽ tự động convert mô hình sang dạng C-array và lưu đè file `TinyML.h` trực tiếp vào thư mục `include/` của Project PlatformIO.

### Bước 4: Triển khai lên ESP32-S3 (Deployment)
Quá trình deploy hoàn toàn tự động nhờ sự liên kết thư mục ở Bước 3.
1. Mở project trên **VS Code (PlatformIO)**.
2. Chọn đúng môi trường build ở thanh trạng thái bên dưới (ví dụ: `env:node` dành cho Sensor Node).
3. Nhấn **Build** và **Upload**. ESP32-S3 sẽ ngay lập tức chạy mô hình AI với tệp tạ giá trị trọng số mới nhất.

---

## 5. Thông tin nhóm
* **Thành viên 1:** Phạm Gia Lương - 2211960 - luong.pham2211960@hcmut.edu.vn
* **Thành viên 2:** Lê Quang Minh - 2212047 - minh.lelight@hcmut.edu.vn
```