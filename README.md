# HỆ THỐNG GIÁM SÁT CHUỖI CUNG ỨNG LẠNH (CCMS)
### Cold Chain Monitoring System
**IoT Assignment Project: RTOS-based Embedded System on ESP32-S3**
---

## Mục lục (Table of Contents)
- [1. Giới thiệu tổng quan (Project Overview)](#1-giới-thiệu-tổng-quan-project-overview)
  - [Danh sách các Task (Project Tasks)](#danh-sách-các-task-project-tasks)
- [2. Cấu trúc mã nguồn (Project Structure)](#2-cấu-trúc-mã-nguồn-project-structure)
  - [2.1. Quản lý thư viện bên ngoài (External Libraries)](#21-quản-lý-thư-viện-bên-ngoài-external-libraries)
  - [2.2. Tổ chức Source Code (Source & Header Separation)](#22-tổ-chức-source-code-source--header-separation)
  - [2.3. Đồng bộ Format Code (Code Consistency)](#23-đồng-bộ-format-code-code-consistency)
- [3. Quy tắc làm việc chung](#3-quy-tắc-làm-việc-chung)
  - [3.1. Quy tắc làm việc với Git](#31-quy-tắc-làm-việc-với-git)
  - [3.2. Quy tắc Code (Coding Standards)](#32-quy-tắc-code-coding-standards)
- [4. Hướng dẫn Thu thập dữ liệu, Huấn luyện và Deploy Mô hình TinyML (MLOps)](#4-hướng-dẫn-thu-thập-dữ-liệu-huấn-luyện-và-deploy-mô-hình-tinyml-mlops)
- [5. Hướng dẫn Biên dịch và Nạp Code (Build & Upload Guide)](#5-hướng-dẫn-biên-dịch-và-nạp-code-build--upload-guide)
  - [5.1. Nạp Firmware cho Sensor Node](#51-nạp-firmware-cho-sensor-node)
  - [5.2. Nạp Firmware và Dữ liệu Web cho Gateway Node](#52-nạp-firmware-và-dữ-liệu-web-cho-gateway-node)
- [6. Quy trình Kiểm thử (Testing Workflow)](#6-quy-trình-kiểm-thử-testing-workflow)
- [7. Thông tin nhóm](#7-thông-tin-nhóm)
---

## 1. Giới thiệu tổng quan (Project Overview)

**Cold Chain Monitoring System (CCMS)** là giải pháp công nghệ được thiết kế chuyên biệt để giám sát, cảnh báo và quản lý các điều kiện môi trường (nhiệt độ, độ ẩm) tại các buồng lạnh công nghiệp. Việc duy trì nghiêm ngặt các dải thông số môi trường tiêu chuẩn đóng vai trò sống còn trong việc bảo vệ chất lượng và tuổi thọ của các loại hàng hóa nhạy cảm (như nông sản, thực phẩm đông lạnh, dược phẩm hay vắc-xin) trong suốt quá trình lưu trữ.

Dự án này được phát triển như một bài tập lớn cho môn học Phát triển ứng dụng IoT (CO3037), tập trung vào việc hiện thực hóa hệ thống nhúng thời gian thực (Real-time embedded systems) trên nền tảng vi điều khiển ESP32-S3 thông qua môi trường PlatformIO.

Mục tiêu cốt lõi của dự án là xây dựng một hệ thống hoàn chỉnh hoạt động trên hệ điều hành FreeRTOS, mở rộng và thay đổi ít nhất 30% so với mã nguồn gốc được cung cấp. Hệ thống tích hợp toàn diện các chức năng: thu thập dữ liệu cảm biến, hiển thị trạng thái qua LCD/NeoPixel, quản trị hệ thống qua Web Server cục bộ, và ứng dụng trí tuệ nhân tạo biên (Edge AI - TinyML) để nhận diện tự động các sự cố. 

Điểm nhấn công nghệ đột phá của dự án nằm ở **kiến trúc mạng đa nút (Multi-node) phân cấp**. Hệ thống sử dụng mạng vô tuyến nội bộ dựa trên giao thức ESP-NOW để kết nối linh hoạt các thiết bị cảm biến (Sensor Nodes) đặt bên trong nhiều buồng lạnh khác nhau, sau đó tổng hợp luồng thông tin về một thiết bị trung tâm (Gateway Node) nhằm đồng bộ dữ liệu lên nền tảng đám mây CoreIoT thông qua kết nối Wi-Fi và MQTT.

### Danh sách các Task (Project Tasks)

Dưới đây là các nhiệm vụ chính cần thực hiện. Các thành viên cập nhật chi tiết tính năng vào phần "Description" khi triển khai.

#### Task 1: Single LED Blink with Temperature Conditions 
* **Mô tả:** Điều khiển hành vi nháy LED cảnh báo, được thiết kế tách biệt hoàn toàn thành 2 module để phục vụ cho kiến trúc Gateway và Sensor Node.
* **Chi tiết tính năng:**
    * **Logic nháy LED Sensor Node:** Cảnh báo các bất thường tại buồng lạnh. Trạng thái Normal (Nháy chậm: 500ms ON / 2500ms OFF). Trạng thái Warning (Nháy vừa: Cảnh báo nhiệt độ/độ ẩm vượt ngưỡng hoặc AI phát hiện cửa mở quá lâu). Trạng thái Critical (Nháy nhanh: Lỗi mất kết nối cảm biến DHT20 hoặc màn hình LCD).
    * **Logic nháy LED Gateway Node:** Cảnh báo tình trạng mạng trung tâm. Trạng thái Normal (Đã kết nối CoreIoT an toàn). Trạng thái Warning (Đang ở chế độ phát WiFi - AP Mode). Trạng thái Critical (Mất kết nối WiFi hoặc rớt mạng MQTT Cloud).
    * **Cơ chế Semaphore sử dụng:** Sử dụng 2 Binary Semaphore độc lập (sensor_led_sync_semaphore và gw_led_sync_semaphore). Thay vì Polling tốn CPU, các Task giám sát khi phát hiện lỗi sẽ xSemaphoreGive để đánh thức lập tức Task LED, ép nó đổi chu kỳ chớp tắt theo thời gian thực (Real-time).

#### Task 2: NeoPixel LED Control Based on Humidity
* **Mô tả:** Điều khiển màu sắc LED NeoPixel dựa trên độ ẩm tại Sensor Node.
* **Chi tiết tính năng:**
    * [TODO: Mapping dải độ ẩm - màu sắc]
    * [TODO: Logic đồng bộ hóa Semaphore]

#### Task 3: Temperature and Humidity Monitoring with LCD
* **Mô tả:** Hiển thị thông tin lên LCD tại Sensor Node, quản lý trạng thái hiển thị (Normal/Warning/Critical) và loại bỏ biến toàn cục.
* **Chi tiết tính năng:**
    * [TODO: Các trạng thái hiển thị]
    * [TODO: Phương pháp loại bỏ global variables]

#### Task 4: Web Server in Access Point Mode
* **Mô tả:** Xây dựng Web Server chạy trên ESP32 (Gateway) đóng vai trò là Captive Portal quản lý tập trung toàn bộ mạng lưới (Multi-node Network) qua giao diện SPA (Single Page Application).
* **Chi tiết tính năng:**
    * **Dashboard:** Hiển thị trạng thái chung của Gateway và tạo các thẻ (Card) giám sát độc lập cho từng Sensor Node (Nhiệt độ, Độ ẩm, Peripheral, AI Prediction). Tích hợp biểu đồ lịch sử Dual Y-Axis (sử dụng Canvas HTML5 thuần không cần Internet) vẽ 30 điểm dữ liệu gần nhất của từng buồng lạnh.
    * **Điều khiển (Controls):** Giao diện điều khiển Relay thực tế (Còi cảnh báo & Quạt thông gió), nút ép buộc Gateway chuyển sang chế độ WiFi (Station Mode), và đặc biệt có thêm tính năng Node Remote Control để gửi lệnh Reset phần cứng từ xa xuống từng Sensor Node thông qua giao thức ESP-NOW.
    * **Cấu hình hệ thống (Settings):** Lưu trữ Flash cho cấu hình Mạng (AP, WiFi) và Cloud (MQTT CoreIoT). Cung cấp giao diện Quản lý Node (Pair/Unpair bằng MAC Address) và cho phép cấu hình đồng bộ ngưỡng cảnh báo (Min/Max Temp/Hum) riêng biệt xuống từng Node con thông qua ESP-NOW.

#### Task 5: TinyML Deployment & Accuracy Evaluation
* **Mô tả:** Triển khai Edge AI tại Sensor Node để phân loại trạng thái buồng lạnh thành 3 nhãn: Bình thường (Normal), Mở cửa nhập/xuất hàng (Restock), và Lỗi hệ thống (Anomaly).
* **Chi tiết tính năng:**
    * **Tập Dataset & Kỹ thuật Feature Engineering:** Dữ liệu được thu thập trực tiếp từ ESP32 qua Serial bằng script Python. Thay vì học giá trị thô, mô hình được huấn luyện bằng 4 đặc trưng: Giá trị chuẩn hóa (t_norm, h_norm - giúp mô hình chạy chuẩn xác với mọi buồng lạnh mà không phụ thuộc vào ngưỡng cài đặt) và Tốc độ thay đổi (t_speed, h_speed - giúp phân biệt việc cửa mở đột ngột so với máy lạnh rò rỉ hư hỏng từ từ).
    * **Cơ chế Debounce trên phần cứng:** Kết hợp kết quả phân loại từ TFLite Micro với logic quản lý trạng thái trên C++. Khi AI báo "Door Open", hệ thống không báo động ngay mà bắt đầu đếm ngược (Debounce ~ 20 giây). Nếu cửa vẫn mở quá thời gian quy định, cờ Semaphore cảnh báo mới được kích hoạt. Quá trình Inference tốn chưa đến 1ms và cấp phát bộ nhớ Arena Size chỉ khoảng 8KB RAM.

#### Task 6: Data Publishing to CoreIOT Cloud Server
* **Mô tả:** Gateway tổng hợp dữ liệu từ các Sensor Node qua ESP-NOW và đẩy lên CoreIOT Server qua WiFi (Station Mode) bằng MQTT.
* **Chi tiết tính năng:**
    * [TODO: Cấu hình xác thực Token]
    * [TODO: Tần suất gửi dữ liệu]

---

## 2. Cấu trúc mã nguồn (Project Structure)

Dự án được xây dựng trên **PlatformIO**, quản lý cả hai luồng thiết bị (Sensor và Gateway) trên cùng một bộ mã nguồn thông qua tính năng Multi-Environment.

```text
.
├── boards/                # Chứa file định nghĩa board
├── data/                  # Chứa các file mã nguồn nạp vào phân vùng LittleFS cho Web Server
├── include/               # Chứa các file header (.h)
├── lib/                   # Kho lưu trữ các thư viện bên NGOÀI (bản Local)
├── src/                   # Source code chính (.cpp)
│   └── main.cpp           # Entry point (Điều hướng Task dựa trên Environment)
├── test/                  # Các bài test: Unit test và Integration test
├── tinyml/                # Source thu thập/đánh nhãn dữ liệu, huấn luyện và export AI model
├── .clang-format          # File quy tắc định dạng code (Google Style)
├── .gitignore             # Cấu hình file/folder cần bỏ qua trong git
├── platformio.ini         # Cấu hình Multi-Environment (sensor_node & gateway_node)
└── README.md              # Tài liệu hướng dẫn dự án
```

### 2.1. Quản lý thư viện bên ngoài (External Libraries)

Để đảm bảo tính ổn định và tránh lỗi do cập nhật phiên bản từ remote, **KHÔNG** sử dụng đường dẫn thư viện online trực tiếp.

* **Vị trí:** Tất cả thư viện bên ngoài phải được tải về và lưu trữ bản local trong thư mục `lib/`.
* **Quy tắc đặt tên:** Thư mục thư viện phải tuân thủ format: `[Tên-Gốc]-v[Phiên-bản]`.
* *_Ví dụ:_* `ArduinoHttpClient-v0.6.1`, `DHT-AdafruitLibrary-v1.4.6`.
* **Cấu hình:** Khai báo thư viện trong `platformio.ini` tại mục `lib_deps` của từng Environment để trình biên dịch nhận diện chính xác phiên bản local.

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

* `Feat`: Thêm tính năng mới.
* `Test`: Thêm bài test mới.
* `Fix`: Sửa lỗi, fig bug sau test.
* `Docs`: Cập nhật tài liệu, comment.
* `Refactor`: Tối ưu cấu trúc code (không thay đổi chức năng).
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

Chạy các lệnh sau tương ứng với từng kịch bản (thay `COM3` bằng cổng thực tế):

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

---

## 5. Hướng dẫn Biên dịch và Nạp Code (Build & Upload Guide)

Để tối ưu hóa tài nguyên phần cứng (Memory & Flash) mà không phải tách project thành 2 thư mục riêng biệt gây khó khăn khi đồng bộ, dự án sử dụng tính năng **Multiple Environments** của PlatformIO. Khi biên dịch, hệ thống sẽ tự động gạt bỏ các thư viện và file mã nguồn không cần thiết của Node đối diện.

### 5.1. Nạp Firmware cho Sensor Node
Sensor Node chịu trách nhiệm đọc DHT20, hiển thị LCD, điều khiển NeoPixel và chạy mô hình TinyML. 

1. Cắm cáp kết nối ESP32 (Sensor Node) vào máy tính.
2. Trên thanh trạng thái (Status Bar) màu xanh dương dưới cùng của VS Code, nhấp vào tên Environment hiện tại (ví dụ: `env:gateway_node` hoặc `Default`).
3. Chọn môi trường **`env:sensor_node`** từ danh sách thả xuống.
4. Nhấn nút **Upload** (Biểu tượng mũi tên sang phải `→` ở thanh công cụ phía dưới). Trình biên dịch sẽ chỉ đóng gói các thư viện liên quan đến Sensor và nạp xuống mạch.

### 5.2. Nạp Firmware và Dữ liệu Web cho Gateway Node
Gateway Node chịu trách nhiệm phát Wi-Fi (AP Mode), host trang Web Dashboard (Captive Portal), quản lý giao tiếp mạng ESP-NOW đa điểm và kết nối MQTT.

1. Cắm cáp kết nối ESP32 (Gateway Node) vào máy tính.
2. Đổi Environment sang **`env:gateway_node`** ở thanh trạng thái bên dưới.
3. **Quan trọng (Nạp Filesystem):** Trước khi nạp code C++, phải nạp giao diện Web tĩnh (HTML/CSS/JS) vào phân vùng LittleFS của ESP32.
   * Nhấp vào biểu tượng con kiến (PlatformIO Logo) ở thanh công cụ bên trái VS Code.
   * Dưới mục `env:gateway_node` -> Mở rộng mục `Platform`.
   * Nhấp vào tác vụ **`Build Filesystem Image`**, sau khi hoàn tất, nhấp tiếp **`Upload Filesystem Image`**.
4. Sau khi nạp Filesystem thành công, nhấn nút **Upload** (Mũi tên sang phải `→` ở thanh công cụ phía dưới) để nạp Firmware C++ cho Gateway.

---

## 6. Quy trình Kiểm thử (Testing Workflow)
Để đảm bảo chất lượng hệ thống và dễ dàng theo dõi tiến độ, nhóm áp dụng quy trình kiểm thử kết hợp giữa việc thiết kế test case (Manual Documenting) và chạy test tự động (Automated PIO Testing) bằng framework Unity.

Dự án sử dụng mô hình **Test Runner Pattern**, trong đó mỗi Node sẽ có một file `test_main.cpp` duy nhất đóng vai trò điều phối các bài test.

### Bước 1: Định nghĩa Test Case (Define Test Cases)
Trước khi viết bất kỳ dòng code test nào **phải** định nghĩa kịch bản test vào sheet `Testing`.
* **Các trường thông tin bắt buộc cần điền trước:**
  * `Type`: Unit / Integration / System
  * `Node`: Sensor / Gateway
  * `ID`: Mã test case (Ví dụ: `US-01`, `UG-01`, `IS-01`...)
  * `Test name`: Tên hàm test (bắt buộc bắt đầu bằng `test_`, ví dụ: `test_set_and_clear_sensor_error_flag`)
  * `Module`: Tên module đang test (Ví dụ: Global, Config, NodeManage...)
  * `Input`: Mô tả thao tác/Dữ liệu đầu vào
  * `Output (Expected)`: Kết quả mong đợi

### Bước 2: Hiện thực Test Code (Implement Test Code)
Sau khi thiết kế kịch bản thì tiến hành code bài test. Quá trình này gồm 2 thao tác bắt buộc:

**2.1. Viết logic bài test (Không chứa setup/loop)**
Viết hàm test sử dụng các macro của Unity (`TEST_ASSERT_TRUE()`, `TEST_ASSERT_EQUAL()`...) tại file tương ứng:
* **Sensor Node:**
  * Unit Test: Viết tại `test/test_sensor/test_sensor_unit.cpp`
  * Integration Test: Viết tại `test/test_sensor/test_sensor_integration.cpp`
* **Gateway Node:**
  * Unit Test: Viết tại `test/test_gateway/test_gateway_unit.cpp`
  * Integration Test: Viết tại `test/test_gateway/test_gateway_integration.cpp`

*Lưu ý: Tuyệt đối KHÔNG viết các hàm `setup()`, `loop()`, `setUp()`, `tearDown()` vào các file này để tránh lỗi trùng lặp (multiple definition).*

**2.2. Khai báo vào Test Runner (`test_main.cpp`)**
Mở file `test_main.cpp` của Node tương ứng (`test/test_sensor/test_main.cpp` hoặc `test/test_gateway/test_main.cpp`), thực hiện 2 việc:
1. Dùng từ khóa `extern` để khai báo tên hàm test ở đầu file.
2. Gọi hàm `RUN_TEST(tên_hàm_test);` bên trong block `setup()` của file `test_main.cpp`.

### Bước 3: Biên dịch và Chạy Test (Build & Run Tests)
*Đảm bảo đã cắm đúng mạch ESP32 tương ứng vào máy tính trước khi chạy.*

**Chạy test cho Sensor Node:**
Chỉ các file trong thư mục `test/test_sensor/` mới được biên dịch. Mở Terminal và gõ:
```bash
pio test -e sensor_node
```

**Chạy test cho Gateway Node:**
Chỉ các file trong thư mục `test/test_gateway/` mới được biên dịch. Mở Terminal và gõ:
```bash
pio test -e gateway_node
```

### Bước 4: Cập nhật Kết quả (Update Test Results)
Sau khi Terminal báo `PASSED` hoặc `FAILED` (màu xanh/đỏ):
* Quay lại sheet `Testing`.
* Cập nhật các cột: `Result` (Pass/Fail), `Tester` (Tên người chạy test), `Testdate` (Ngày chạy), và `Ghi chú` (Nếu có lỗi hoặc điều kiện bất thường để team cùng review).

---

## 7. Thông tin nhóm
* **Thành viên 1:** Phạm Gia Lương - 2211960 - luong.pham2211960@hcmut.edu.vn
* **Thành viên 2:** Lê Quang Minh - 2212047 - minh.lelight@hcmut.edu.vn
