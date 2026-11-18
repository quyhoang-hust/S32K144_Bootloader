# S32K144 Mini Bootloader - Thuật ngữ Anh - Việt

> **Mục đích:** Tổng hợp thuật ngữ kỹ thuật sử dụng trong dự án **S32K144 Mini Bootloader - Fresher 03**, phục vụ đọc source code, viết báo cáo, thuyết trình, review thiết kế và kiểm thử trên board.

---

## Quy ước sử dụng

- Giữ nguyên tên module, API, enum, macro và thanh ghi trong source code, ví dụ: `FlashEngine_MainFunction()`, `BOOT_STATE_MENU`, `FTFC->FSTAT`.
- Bản dịch tiếng Việt dùng để giải thích trong tài liệu và thuyết trình, không dùng thay identifier trong mã nguồn.
- `Application` được giữ nguyên trong nhiều mô tả để phân biệt rõ với Bootloader.
- `Phrase`, `sector`, `job`, `payload`, `state` có thể giữ nguyên khi trao đổi kỹ thuật, kèm nghĩa tiếng Việt ở lần xuất hiện đầu tiên.

## Mục lục

1. [Thuật ngữ tổng quan Bootloader](#1-thuật-ngữ-tổng-quan-bootloader)
2. [Thuật ngữ SW2 và GPIO](#2-thuật-ngữ-sw2-và-gpio)
3. [UART và truyền nhận dữ liệu](#3-uart-và-truyền-nhận-dữ-liệu)
4. [Motorola S-record](#4-motorola-s-record)
5. [Flash và FTFC](#5-flash-và-ftfc)
6. [Thực thi từ RAM và Linker](#6-thực-thi-từ-ram-và-linker)
7. [Flash Job Queue](#7-flash-job-queue)
8. [FlashEngine và Phrase Assembly](#8-flashengine-và-phrase-assembly)
9. [Hai State Machine nội bộ của FlashEngine](#9-hai-state-machine-nội-bộ-của-flashengine)
10. [Bootloader State Machine](#10-bootloader-state-machine)
11. [Scheduler và bốn Task](#11-scheduler-và-bốn-task)
12. [Fatal Error Policy](#12-fatal-error-policy)
13. [Progress và Statistics](#13-progress-và-statistics)
14. [AppValidator và Vector Table](#14-appvalidator-và-vector-table)
15. [JumpManager và ARM Cortex-M4](#15-jumpmanager-và-arm-cortex-m4)
16. [Clock và Watchdog](#16-clock-và-watchdog)
17. [Công cụ phía PC](#17-công-cụ-phía-pc)
18. [Thuật ngữ thiết kế và tài liệu](#18-thuật-ngữ-thiết-kế-và-tài-liệu)
19. [Build và Debug](#19-build-và-debug)
20. [Từ viết tắt quan trọng](#20-từ-viết-tắt-quan-trọng)
21. [Cụm từ thường dùng khi thuyết trình](#21-cụm-từ-thường-dùng-khi-thuyết-trình)

---

## 1. Thuật ngữ tổng quan Bootloader

| English term | Tiếng Việt | Ý nghĩa trong dự án |
|---|---|---|
| Bootloader | Bộ nạp khởi động | Chương trình chạy đầu tiên sau reset, quyết định cập nhật firmware hoặc chạy Application. |
| Mini Bootloader | Bộ nạp khởi động tối giản | Bootloader có phạm vi giới hạn: erase, nhận SREC, program và jump. |
| Application | Chương trình ứng dụng | Firmware chính được Bootloader nạp và chạy. |
| Firmware | Phần sụn | Chương trình nhúng chạy trên vi điều khiển. |
| Bootstrap | Quá trình khởi động ban đầu | Giai đoạn khởi tạo và quyết định Bootloader/Application. |
| Boot flow | Luồng khởi động | Trình tự từ reset đến Bootloader hoặc Application. |
| Boot mode | Chế độ Bootloader | Chế độ cho phép cập nhật firmware. |
| Application mode | Chế độ ứng dụng | Chế độ chạy firmware Application. |
| Boot selection | Lựa chọn chế độ khởi động | Quyết định dựa trên SW2, UART hoặc tính hợp lệ của Application. |
| Boot entry | Yêu cầu vào Bootloader | Sự kiện giữ SW2 hoặc gửi byte UART để ở lại Bootloader. |
| Boot request | Yêu cầu vào chế độ Bootloader | Tín hiệu yêu cầu không jump Application. |
| Entry window | Cửa sổ vào Bootloader | Khoảng thời gian hữu hạn để nhận yêu cầu UART. |
| UART recovery | Khôi phục qua UART | Cơ chế phụ giữ board trong Bootloader để cứu hộ. |
| Mode selector | Bộ chọn chế độ | SW2 dùng để chọn Bootloader hoặc Application. |
| Hand-off | Bàn giao quyền điều khiển | Bootloader chuyển CPU sang Application. |
| Jump to Application | Chuyển sang Application | Đổi VTOR, MSP và branch tới Reset Handler của Application. |
| Valid Application | Application hợp lệ | Có MSP và Reset Vector đạt điều kiện kiểm tra. |
| Invalid Application | Application không hợp lệ | Vector table trống, sai MSP, sai Thumb bit hoặc sai vùng Reset Handler. |
| Erased Application | Application đã bị xóa | Vùng Application chứa giá trị erased, thường là `0xFF`. |
| Recovery mechanism | Cơ chế khôi phục | Phương án vào Bootloader khi Application lỗi. |
| Fallback | Cơ chế dự phòng | Khi Application không hợp lệ, hệ thống quay về Bootloader. |
| Cold reset | Reset khi khởi động mới | MCU khởi động lại từ đầu, thường sau cấp nguồn. |
| Software reset | Reset bằng phần mềm | Reset được kích hoạt bởi firmware. |
| Hardware reset | Reset phần cứng | Reset do nút RESET hoặc tín hiệu ngoài. |
| Reset source | Nguồn gây reset | Nguyên nhân reset được đọc từ `RCM->SRS`. |

---

## 2. Thuật ngữ SW2 và GPIO

| English term | Tiếng Việt | Ý nghĩa |
|---|---|---|
| Boot switch | Công tắc chọn chế độ khởi động | SW2 dùng để chọn Bootloader. |
| Push button | Nút nhấn | Công tắc tác động tạm thời. |
| Pressed / Held | Được nhấn / đang được giữ | SW2 đang nối BTN0 lên VDD. |
| Released | Được thả | SW2 hở, BTN0 bị R44 kéo xuống GND. |
| Raw level | Mức điện thô | Giá trị HIGH/LOW đọc trực tiếp từ GPIO. |
| Debounced decision | Kết quả sau khử dội | HELD hoặc RELEASED sau nhiều lần lấy mẫu. |
| Active-HIGH | Tác động mức cao | HIGH được hiểu là nút đang nhấn. |
| Active-LOW | Tác động mức thấp | LOW được hiểu là nút đang nhấn. |
| Pull-up resistor | Điện trở kéo lên | Kéo tín hiệu về VDD khi không có nguồn điều khiển khác. |
| Pull-down resistor | Điện trở kéo xuống | Kéo tín hiệu về GND khi không nhấn. |
| External pull-down | Điện trở kéo xuống ngoài | R44 10 kΩ trên schematic. |
| Internal pull resistor | Điện trở kéo nội | Điện trở tích hợp trong MCU. |
| Floating input | Đầu vào thả nổi | Input không có mức logic xác định. |
| GPIO | Vào/ra đa dụng | Chân số có thể cấu hình input hoặc output. |
| Input pin | Chân đầu vào | Chân dùng để đọc SW2. |
| Output pin | Chân đầu ra | Chân điều khiển LED. |
| Pin mux / Multiplexer | Bộ chọn chức năng chân | Chọn GPIO hoặc chức năng ngoại vi. |
| Port Control Register | Thanh ghi điều khiển cổng | PCR cấu hình MUX, pull resistor và thuộc tính chân. |
| PDIR | Thanh ghi dữ liệu đầu vào | Thanh ghi đọc trạng thái chân GPIO. |
| PDOR | Thanh ghi dữ liệu đầu ra | Thanh ghi giữ mức output. |
| PSOR | Thanh ghi đặt bit output | Ghi 1 để set output. |
| PCOR | Thanh ghi xóa bit output | Ghi 1 để clear output. |
| PTOR | Thanh ghi đảo bit output | Ghi 1 để toggle output. |
| Clock gate | Cổng cấp clock | Cho phép clock tới PORT hoặc peripheral. |
| Debounce | Khử dội nút nhấn | Loại bỏ dao động ngắn khi đóng/mở switch. |
| Majority vote | Biểu quyết đa số | Quyết định dựa trên phần lớn mẫu đọc. |
| Settling time | Thời gian ổn định | Thời gian chờ tín hiệu GPIO/RC ổn định. |
| RC network | Mạch điện trở - tụ điện | R44 và C16 tạo bộ lọc phần cứng. |
| Time constant | Hằng số thời gian | `τ = R × C` của mạch RC. |

### Trạng thái SW2 đúng theo schematic

```text
SW2 released → BTN0 = LOW  → RELEASED → chạy Application
SW2 held     → BTN0 = HIGH → HELD     → vào Bootloader
```

---

## 3. UART và truyền nhận dữ liệu

| English term | Tiếng Việt | Ý nghĩa |
|---|---|---|
| UART | Giao tiếp nối tiếp không đồng bộ | Kênh truyền SREC giữa PC và board. |
| LPUART | UART công suất thấp | Peripheral UART trên S32K144. |
| Baud rate | Tốc độ baud | Dự án dùng 115200 baud. |
| Data bits | Số bit dữ liệu | Dự án dùng 8 bit dữ liệu. |
| Stop bit | Bit dừng | Đánh dấu kết thúc một khung UART. |
| Parity | Bit chẵn lẻ | Cơ chế phát hiện lỗi đơn giản. |
| 115200-8-N-1 | Cấu hình UART | 115200 baud, 8 data bit, không parity, 1 stop bit. |
| UART RX | Nhận UART | Đường dữ liệu từ PC vào board. |
| UART TX | Truyền UART | Đường dữ liệu từ board tới PC. |
| Receive | Nhận | Nhận byte từ host. |
| Transmit | Truyền | Gửi log hoặc phản hồi tới host. |
| RX interrupt | Ngắt nhận UART | Ngắt khi có dữ liệu UART tới. |
| Interrupt Service Routine | Trình phục vụ ngắt | Hàm xử lý ngắt, viết tắt ISR. |
| Polling | Thăm dò | Liên tục kiểm tra một cờ trạng thái. |
| TX polling | Thăm dò truyền | Chờ thanh ghi UART sẵn sàng gửi. |
| Ring buffer | Bộ đệm vòng | Buffer FIFO dùng cho UART RX. |
| Producer | Bên sản xuất dữ liệu | UART ISR đưa byte vào RingBuffer. |
| Consumer | Bên tiêu thụ dữ liệu | Menu task hoặc SREC task lấy byte ra. |
| Push | Đưa dữ liệu vào | Thêm byte/job vào buffer hoặc queue. |
| Pop | Lấy và xóa dữ liệu đầu | Đọc rồi loại phần tử đầu. |
| Peek | Xem phần tử đầu | Đọc nhưng chưa xóa. |
| Buffer overflow | Tràn bộ đệm | Dữ liệu tới khi buffer không còn chỗ. |
| Stale data | Dữ liệu tồn đọng | Byte cũ còn lại từ phiên trước. |
| Clear RX flags | Xóa cờ nhận UART | Xóa trạng thái lỗi hoặc pending của UART. |
| Entry byte | Byte yêu cầu vào Bootloader | Byte bất kỳ gửi trong UART entry window. |
| Carriage Return | Ký tự về đầu dòng | `CR`, mã `\r`. |
| Line Feed | Ký tự xuống dòng | `LF`, mã `\n`. |
| Serial port | Cổng nối tiếp | Ví dụ `COM13`. |
| Host | Máy tính điều khiển | Máy tính gửi SREC và nhận log. |
| Target | Thiết bị đích | Board S32K144. |
| Streaming | Truyền liên tục | Gửi tuần tự các dòng SREC qua UART. |
| Receive window | Cửa sổ nhận | Khoảng thời gian chấp nhận dữ liệu. |
| UART entry request | Yêu cầu vào Bootloader qua UART | Recovery mechanism bằng script. |

---

## 4. Motorola S-record

| English term | Tiếng Việt | Ý nghĩa |
|---|---|---|
| S-record / SREC | Định dạng Motorola S-record | File văn bản chứa địa chỉ, dữ liệu và checksum. |
| Record | Bản ghi | Một dòng SREC hoàn chỉnh. |
| Record type | Loại bản ghi | S0 đến S9. |
| Record class | Nhóm bản ghi | DATA, INFO, END hoặc UNSUPPORTED. |
| Header record | Bản ghi tiêu đề | S0. |
| Data record | Bản ghi dữ liệu | S1, S2 hoặc S3. |
| Count record | Bản ghi đếm | S5 hoặc S6. |
| Termination record | Bản ghi kết thúc | S7, S8 hoặc S9. |
| Informational record | Bản ghi thông tin | S0, S5, S6. |
| Unsupported record | Bản ghi không hỗ trợ | S4 hoặc loại không hợp lệ. |
| Reserved record | Bản ghi dành riêng | S4, không được dùng làm payload. |
| Byte count | Số byte được đếm | Bao gồm address, data và checksum. |
| Address field | Trường địa chỉ | Địa chỉ dữ liệu hoặc entry point. |
| Payload | Dữ liệu tải | Các byte thực cần ghi vào Flash. |
| Data length | Độ dài dữ liệu | Số byte payload, không gồm address/checksum. |
| Hexadecimal | Hệ thập lục phân | Biểu diễn byte bằng hai ký tự hex. |
| Hex nibble | Nửa byte hex | Một ký tự `0-9`, `A-F` hoặc `a-f`. |
| Checksum | Tổng kiểm tra | Phát hiện lỗi trên một record. |
| Checksum validation | Kiểm tra checksum | Xác nhận byte thấp của tổng bằng `0xFF`. |
| Parse | Phân tích cú pháp | Chuyển dòng ký tự thành record có cấu trúc. |
| Parser | Bộ phân tích cú pháp | Module `SrecParser`. |
| Line builder | Bộ ghép dòng | Ghép byte RX thành một dòng SREC. |
| Input filter | Bộ lọc đầu vào | Chặn ký tự điều khiển không hợp lệ. |
| Invalid format | Sai định dạng | Dòng không bắt đầu bằng `S` hoặc type sai. |
| Invalid hex | Ký tự hex không hợp lệ | Có ký tự ngoài tập hex. |
| Invalid length | Độ dài không hợp lệ | Độ dài dòng không khớp byte count. |
| Invalid byte count | Số byte không hợp lệ | Count nhỏ hơn address + checksum. |
| Invalid checksum | Checksum không hợp lệ | Tổng kiểm tra không bằng `0xFF`. |
| Termination pair | Cặp data/end record | S1/S9, S2/S8, S3/S7. |
| End record | Bản ghi kết thúc | Báo SREC stream đã hết. |
| Maximum line length | Độ dài dòng tối đa | Dự án cấu hình 520 ký tự. |
| Maximum data size | Kích thước dữ liệu tối đa | Tối đa 252 byte với S1. |

### Ý nghĩa các loại SREC

| Loại | Tiếng Việt | Số byte địa chỉ | Xử lý |
|---|---|---:|---|
| S0 | Bản ghi tiêu đề | 2 | Kiểm tra rồi bỏ qua. |
| S1 | Dữ liệu địa chỉ 16-bit | 2 | Tạo program job. |
| S2 | Dữ liệu địa chỉ 24-bit | 3 | Tạo program job. |
| S3 | Dữ liệu địa chỉ 32-bit | 4 | Tạo program job. |
| S4 | Loại dành riêng | Không dùng | Fatal error. |
| S5 | Đếm record 16-bit | 2 | Kiểm tra rồi bỏ qua. |
| S6 | Đếm record 24-bit | 3 | Kiểm tra rồi bỏ qua. |
| S7 | Kết thúc, địa chỉ 32-bit | 4 | Tạo `END_DOWNLOAD`. |
| S8 | Kết thúc, địa chỉ 24-bit | 3 | Tạo `END_DOWNLOAD`. |
| S9 | Kết thúc, địa chỉ 16-bit | 2 | Tạo `END_DOWNLOAD`. |

---

## 5. Flash và FTFC

| English term | Tiếng Việt | Ý nghĩa |
|---|---|---|
| Flash memory | Bộ nhớ Flash | Bộ nhớ không mất dữ liệu khi mất nguồn. |
| Program Flash / P-Flash | Flash chương trình | Vùng chứa Bootloader và Application. |
| Application region | Vùng Application | Vùng `0x00020000` tới `0x00080000`. |
| Bootloader region | Vùng Bootloader | Vùng từ `0x00000000` tới trước Application. |
| Flash sector | Sector Flash | Đơn vị erase, dự án dùng 4 KB. |
| Flash phrase | Phrase Flash | Đơn vị program, dự án dùng 8 byte. |
| Sector erase | Xóa sector | Đưa sector về trạng thái erased. |
| Program phrase | Ghi phrase | Ghi một khối 8 byte xuống Flash. |
| Erased value | Giá trị sau khi xóa | Thường là `0xFF` cho mỗi byte. |
| FTFC | Bộ điều khiển Flash | Module điều khiển Flash của S32K144. |
| Flash command | Lệnh Flash | Lệnh erase hoặc program gửi tới FTFC. |
| Command code | Mã lệnh | `0x09` erase sector, `0x07` program phrase. |
| Command whitelist | Danh sách lệnh được phép | Chỉ cho phép command đã thiết kế. |
| FCCOB | Thanh ghi đối tượng lệnh Flash | Chứa command, address và program data. |
| FSTAT | Thanh ghi trạng thái Flash | Chứa CCIF và các cờ lỗi. |
| CCIF | Cờ hoàn thành lệnh | `0`: đang xử lý; `1`: đã hoàn thành/sẵn sàng. |
| ACCERR | Lỗi truy cập | Lệnh hoặc truy cập Flash không hợp lệ. |
| FPVIOL | Vi phạm bảo vệ Flash | Ghi/xóa vùng được bảo vệ. |
| RDCOLERR | Lỗi xung đột đọc | Xung đột đọc khi Flash operation. |
| MGSTAT0 | Trạng thái verify thất bại | Flash command không hoàn thành đúng. |
| Flash controller | Bộ điều khiển Flash | Phần cứng thực hiện erase/program. |
| Flash operation | Thao tác Flash | Erase hoặc program. |
| Operation address | Địa chỉ thao tác | Sector hoặc phrase đang xử lý. |
| Hardware status | Trạng thái phần cứng | Giá trị đọc từ FTFC. |
| Flash protection | Bảo vệ Flash | Ngăn ghi/xóa ngoài Application region. |
| Address violation | Vi phạm địa chỉ | Address ngoài vùng cho phép. |
| Alignment violation | Vi phạm căn chỉnh | Address không align sector/phrase. |
| Read-While-Write | Đọc trong khi ghi | Hạn chế khi CPU fetch code từ Flash đang bận. |
| Single partition | Phân vùng Flash đơn | Bootloader và Application nằm cùng P-Flash partition. |
| Critical section | Vùng xử lý tới hạn | Đoạn code mask interrupt và thực hiện Flash command. |
| Command launch | Khởi chạy lệnh | Ghi 1 vào CCIF để bắt đầu command. |
| Launch and wait | Khởi chạy và chờ | Gửi lệnh rồi chờ FTFC hoàn tất. |
| Poll loop | Vòng lặp thăm dò | Vòng `while` chờ CCIF hoàn thành. |
| Flash busy | Flash đang bận | FTFC đang erase/program. |
| Flash ready | Flash sẵn sàng | CCIF cho phép bắt đầu lệnh mới. |
| Synchronous operation | Thao tác đồng bộ | Hàm chỉ return sau khi command hoàn tất. |
| Asynchronous operation | Thao tác bất đồng bộ | Hàm launch rồi return trước khi command xong. |
| Acknowledge result | Xác nhận đã xử lý kết quả | Reset trạng thái Driver về IDLE. |
| Driver-owned buffer | Buffer thuộc sở hữu Driver | Bản copy phrase trong SRAM. |

---

## 6. Thực thi từ RAM và Linker

| English term | Tiếng Việt | Ý nghĩa |
|---|---|---|
| RAM function | Hàm chạy từ RAM | Hàm được đặt trong SRAM để chạy khi Flash bận. |
| `RAM_FUNC` | Thuộc tính hàm chạy từ RAM | Macro gán section attribute. |
| Code section | Phân đoạn mã | Vùng chứa machine code. |
| `.code_ram` | Phân đoạn mã trong RAM | Section chứa các Flash critical function. |
| Load address | Địa chỉ lưu ban đầu | Vị trí code được lưu trong Flash image. |
| Run address | Địa chỉ thực thi | Vị trí code được copy tới và chạy. |
| Linker script | Kịch bản liên kết | File định nghĩa memory layout và section. |
| Linker symbol | Ký hiệu linker | Địa chỉ đầu/cuối section. |
| Startup code | Mã khởi động | Copy data/code RAM và khởi tạo runtime. |
| Copy to RAM | Sao chép sang RAM | Chuyển `.code_ram` từ Flash sang SRAM. |
| Instruction fetch | Đọc lệnh CPU | CPU lấy instruction để thực thi. |
| Memory barrier | Hàng rào bộ nhớ | Ngăn compiler/CPU sắp xếp lại truy cập. |
| Data Synchronization Barrier | Hàng rào đồng bộ dữ liệu | DSB. |
| Instruction Synchronization Barrier | Hàng rào đồng bộ lệnh | ISB. |
| PRIMASK | Thanh ghi khóa ngắt | Điều khiển global interrupt mask. |
| Disable interrupt | Vô hiệu hóa ngắt | Lệnh `cpsid i`. |
| Restore interrupt | Khôi phục ngắt | Trả PRIMASK về trạng thái trước. |
| Inline assembly | Hợp ngữ nội tuyến | Lệnh ARM viết trong C. |
| No-return function | Hàm không quay lại | Hàm jump Application. |
| `NO_RETURN` | Thuộc tính không return | Báo compiler rằng hàm không trở về. |
| No-inline | Không cho nội tuyến hóa | Giữ hàm trong section mong muốn. |
| Map file | File bản đồ liên kết | Kiểm tra địa chỉ hàm và section. |

---

## 7. Flash Job Queue

| English term | Tiếng Việt | Ý nghĩa |
|---|---|---|
| Job | Công việc / yêu cầu xử lý | Một đơn vị công việc Flash. |
| Flash job | Công việc Flash | Erase, program data hoặc end download. |
| Job queue | Hàng đợi công việc | FIFO nối SREC task với FlashEngine. |
| Queue entry | Phần tử hàng đợi | Một slot chứa Flash job. |
| Queue capacity | Sức chứa hàng đợi | Tổng số slot, dự án dùng 8. |
| Queue usage | Mức sử dụng hàng đợi | Số slot đang chứa job. |
| Peak usage | Mức sử dụng cao nhất | Số slot cao nhất từng dùng trong phiên. |
| Queue overflow | Tràn hàng đợi | Push khi queue đã full. |
| FIFO | Vào trước ra trước | Thứ tự xử lý job. |
| Head index | Chỉ số đầu hàng đợi | Vị trí job được xử lý tiếp theo. |
| Tail index | Chỉ số cuối hàng đợi | Vị trí ghi job mới. |
| Job count | Số lượng job | Số phần tử hiện có. |
| Queue ownership | Quyền sở hữu dữ liệu của queue | Queue giữ bản copy payload. |
| Payload copy | Sao chép dữ liệu tải | `memcpy` vào storage riêng. |
| Durable copy | Bản copy ổn định | Không bị thay đổi bởi parser. |
| Erase job | Job xóa | `FLASH_JOB_ERASE_APP`. |
| Program job | Job ghi dữ liệu | `FLASH_JOB_PROGRAM_DATA`. |
| End-download job | Job kết thúc tải | `FLASH_JOB_END_DOWNLOAD`. |
| Current job | Job hiện tại | Job FlashEngine đang xử lý. |
| Acquire job | Nhận job | Peek job từ queue vào context. |
| Release job | Giải phóng job | Pop sau khi hoàn tất. |

---

## 8. FlashEngine và Phrase Assembly

| English term | Tiếng Việt | Ý nghĩa |
|---|---|---|
| FlashEngine | Bộ máy xử lý Flash | Điều phối job, phrase, gap, padding và progress. |
| Engine mode | Chế độ nội bộ Engine | IDLE, ERASE, ERASE_COMPLETE, DOWNLOAD. |
| Idle | Rỗi | Không có thao tác đang hoạt động. |
| Erase mode | Chế độ xóa | Đang erase Application region. |
| Download mode | Chế độ tải | Đang xử lý program job. |
| Erase complete mode | Chế độ xóa hoàn tất | Mode trung gian để report 100%. |
| Active phrase | Phrase đang hoạt động | Buffer 8 byte đang được gom. |
| Phrase assembly | Ghép phrase | Gom dữ liệu SREC thành khối 8 byte. |
| Phrase base address | Địa chỉ cơ sở phrase | Address align xuống bội số 8. |
| Fill offset | Vị trí điền tiếp theo | `nextFillOffset`. |
| Actual data byte count | Số byte dữ liệu thực | Không bao gồm padding. |
| Submitted | Đã gửi xuống Driver | Phrase không được sửa thêm. |
| Complete phrase | Phrase đầy đủ | Đã có đủ 8 byte kể cả padding. |
| Partial phrase | Phrase chưa đầy | Chưa đủ 8 byte. |
| Leading padding | Padding đầu phrase | `0xFF` trước địa chỉ unaligned. |
| Tail padding | Padding cuối phrase | `0xFF` sau byte data cuối. |
| Gap | Khoảng trống địa chỉ | Vùng giữa các khối dữ liệu. |
| Gap handling | Xử lý khoảng trống | Padding trong phrase hoặc bỏ qua vùng đã erase. |
| Overlap | Chồng lấn địa chỉ | Record mới ghi đè dữ liệu đã consume. |
| Backward address | Địa chỉ đi lùi | Address mới nhỏ hơn expected address. |
| Expected next address | Địa chỉ kế tiếp mong đợi | Dùng kiểm tra continuity/overlap. |
| Continuity | Tính liên tục địa chỉ | Record mới bắt đầu đúng địa chỉ kế tiếp. |
| Unaligned address | Địa chỉ không căn chỉnh | Không chia hết cho 8. |
| Aligned address | Địa chỉ đã căn chỉnh | Chia hết cho sector hoặc phrase size. |
| Consume data | Tiêu thụ dữ liệu | Copy byte job vào active phrase. |
| Flush | Xả dữ liệu còn lại | Hoàn thiện và program phrase cuối. |
| End-download pending | Đang chờ kết thúc tải | END job chưa hoàn tất. |
| Cyclic processing | Xử lý tuần hoàn | Mỗi scheduler cycle làm một bước hữu hạn. |
| Processing context | Ngữ cảnh xử lý | Offset, state, phrase và expected address. |
| Operation budget | Giới hạn thao tác mỗi cycle | Tối đa một job/operation theo thiết kế. |

---

## 9. Hai State Machine nội bộ của FlashEngine

| English term | Tiếng Việt | Ý nghĩa |
|---|---|---|
| State machine | Máy trạng thái | Mô hình trạng thái và chuyển tiếp. |
| Internal state machine | Máy trạng thái nội bộ | State machine bên trong FlashEngine. |
| SREC Job State Machine | Máy trạng thái job SREC | Theo dõi việc consume một data job. |
| Phrase Job State Machine | Máy trạng thái phrase | Theo dõi vòng đời phrase 8 byte. |
| Progressing | Đang xử lý | Job SREC còn byte chưa consume. |
| Done | Hoàn tất | Đơn vị công việc đã hoàn thành. |
| Busy | Đang hoạt động | Phrase đang tồn tại/chưa hoàn tất. |
| State transition | Chuyển trạng thái | Thay đổi từ state này sang state khác. |
| State invariant | Điều kiện bất biến | Điều kiện luôn đúng trong một state. |
| Terminal state | Trạng thái kết thúc | Không tự chuyển đi nếu chưa reset. |
| Current state | Trạng thái hiện tại | State đang có hiệu lực. |
| Previous state | Trạng thái trước đó | Dùng phát hiện state change. |
| Valid transition | Chuyển trạng thái hợp lệ | Chuyển tiếp có trong thiết kế. |
| Invalid transition | Chuyển trạng thái không hợp lệ | Bị từ chối hoặc nâng INTERNAL_STATE. |

### Các trạng thái FlashEngine

| Identifier | Tiếng Việt |
|---|---|
| `FLS_SREC_JOB_PROGRESSING` | Job SREC đang được xử lý. |
| `FLS_SREC_JOB_DONE` | Job SREC đã được consume hết. |
| `FLS_JOB_BUSY` | Phrase đang hoạt động/chưa hoàn tất. |
| `FLS_JOB_DONE` | Phrase đã hoàn tất hoặc không có active phrase. |
| `FLASH_ENGINE_MODE_IDLE` | Engine đang rỗi. |
| `FLASH_ENGINE_MODE_ERASE` | Engine đang xóa. |
| `FLASH_ENGINE_MODE_ERASE_COMPLETE` | Engine vừa xóa xong, chờ report 100%. |
| `FLASH_ENGINE_MODE_DOWNLOAD` | Engine đang xử lý download. |

---

## 10. Bootloader State Machine

| Identifier | Tiếng Việt | Chức năng |
|---|---|---|
| `BOOT_STATE_MENU` | Trạng thái menu | Chờ chọn erase/download. |
| `BOOT_STATE_ERASING_APP` | Đang xóa Application | FlashEngine erase 96 sector. |
| `BOOT_STATE_RECEIVING` | Đang nhận SREC | Nhận, parse và tạo job. |
| `BOOT_STATE_FLUSHING` | Đang xả dữ liệu cuối | Program các job/phrase còn lại. |
| `BOOT_STATE_FINISHED` | Đã tải hoàn tất | Phiên download thành công. |
| `BOOT_STATE_ERROR` | Trạng thái lỗi | Dừng các hoạt động nguy hiểm. |

| English term | Tiếng Việt |
|---|---|
| Bootstrap decision | Quyết định khởi động. |
| State transition table | Bảng chuyển trạng thái. |
| State gating | Giới hạn hành vi theo state. |
| Operational state | Trạng thái hoạt động. |
| Error state | Trạng thái lỗi. |
| Finished state | Trạng thái hoàn tất. |
| State observer | Thành phần quan sát state. |
| State report | Báo cáo trạng thái. |
| Transition allowed | Cho phép chuyển trạng thái. |
| Transition rejected | Từ chối chuyển trạng thái. |
| Stay in Bootloader | Ở lại Bootloader. |
| Enter Application | Chuyển sang Application. |

---

## 11. Scheduler và bốn Task

| English term | Tiếng Việt | Ý nghĩa |
|---|---|---|
| Scheduler | Bộ lập lịch | Gọi tuần tự bốn task. |
| Cyclic scheduler | Bộ lập lịch tuần hoàn | Lặp vô hạn theo thứ tự cố định. |
| Cyclic executive | Bộ điều hành tuần hoàn | Kiến trúc không RTOS. |
| Task | Tác vụ | Hàm thực hiện một nhóm trách nhiệm. |
| Task responsibility | Trách nhiệm tác vụ | Phạm vi công việc task được phép làm. |
| Task execution | Thực thi tác vụ | Một lần task được scheduler gọi. |
| Scheduler cycle | Một vòng scheduler | Một lượt gọi toàn bộ task. |
| Bounded execution | Thực thi có giới hạn | Task không xử lý vô hạn trong một lần. |
| Starvation | Thiếu thời gian thực thi | Một task chiếm CPU làm task khác không chạy. |
| Rate limiting | Giới hạn tần suất | Không report quá thường xuyên. |
| Snapshot | Ảnh chụp trạng thái | Bản copy statistic/error tại thời điểm đọc. |
| State-gated task | Task giới hạn theo state | Chỉ xử lý khi BootloaderState phù hợp. |
| Thin wrapper | Hàm bao mỏng | Task chỉ gọi `FlashEngine_MainFunction()`. |
| Background processing | Xử lý nền | Trong dự án là xử lý tuần hoàn qua scheduler. |

### Bốn Task

| Task | Tiếng Việt |
|---|---|
| `Task_MenuProcess` | Tác vụ xử lý menu. |
| `Task_SrecRxProcess` | Tác vụ nhận và xử lý SREC. |
| `Task_FlashProgram` | Tác vụ xử lý ghi Flash. |
| `Task_ReportStatistic` | Tác vụ báo cáo thống kê. |

---

## 12. Fatal Error Policy

| English term | Tiếng Việt | Ý nghĩa |
|---|---|---|
| Fatal error | Lỗi nghiêm trọng | Lỗi khiến phiên phải dừng. |
| Error policy | Chính sách xử lý lỗi | Quy định lưu, report và phản ứng với lỗi. |
| Error code | Mã lỗi | Loại lỗi. |
| Error address | Địa chỉ liên quan lỗi | Address hoặc diagnostic detail. |
| First-error latch | Chốt lỗi đầu tiên | Không cho lỗi sau ghi đè lỗi gốc. |
| Root cause | Nguyên nhân gốc | Lỗi đầu tiên tạo ra chuỗi lỗi. |
| Secondary error | Lỗi thứ cấp | Lỗi phát sinh từ lỗi trước. |
| Pending error | Lỗi đang chờ xử lý | Lỗi ISR chưa được nâng sang task context. |
| Error snapshot | Ảnh chụp lỗi | Code và address đã lưu. |
| Fail-safe | Trạng thái an toàn khi lỗi | Dừng erase/program/parse. |
| Error recovery | Khôi phục sau lỗi | Reset để bắt đầu lại. |
| Error reporting | Báo cáo lỗi | Gửi `[FATAL]` qua UART. |
| Task context | Ngữ cảnh task | Có thể gọi state machine đầy đủ. |
| ISR context | Ngữ cảnh ngắt | Chỉ publish pending error. |
| Internal state error | Lỗi trạng thái nội bộ | Logic hoặc transition không hợp lệ. |
| Queue overflow error | Lỗi tràn hàng đợi | Không còn slot lưu job. |
| Address out of range | Địa chỉ ngoài phạm vi | Truy cập ngoài Application region. |
| Address overlap | Địa chỉ chồng lấn | Dữ liệu đi lùi hoặc ghi đè. |
| Flash erase failed | Xóa Flash thất bại | FTFC erase báo lỗi. |
| Flash write failed | Ghi Flash thất bại | FTFC program báo lỗi. |
| Invalid SREC | SREC không hợp lệ | Sai format, checksum, hex hoặc record. |
| Line too long | Dòng quá dài | Vượt `SREC_MAX_LINE_LENGTH`. |
| RX overflow | Tràn bộ đệm nhận | RingBuffer không còn chỗ. |

---

## 13. Progress và Statistics

| English term | Tiếng Việt | Ý nghĩa |
|---|---|---|
| Progress | Tiến độ | Mức hoàn thành thao tác. |
| Erase progress | Tiến độ xóa | Tỷ lệ sector đã xóa. |
| Download progress | Tiến độ tải | Số byte data đã ghi thành công. |
| Percentage | Phần trăm | Giá trị từ 0 đến 100. |
| Total sector count | Tổng số sector | 96 sector. |
| Erased sector count | Số sector đã xóa | Chỉ tăng sau erase thành công. |
| Programmed data bytes | Số byte dữ liệu đã ghi | Không tính padding. |
| Programmed phrase count | Số phrase đã ghi | Mỗi phrase thành công tăng một. |
| Completed line count | Số dòng hoàn chỉnh | Dòng SREC đã nhận đủ. |
| Data record count | Số data record | Số S1/S2/S3. |
| End record count | Số end record | Số S7/S8/S9. |
| Reporting step | Bước báo cáo | Ví dụ mỗi 256 byte. |
| Final report | Báo cáo cuối | Được in khi FINISHED. |
| Download summary | Tổng kết phiên tải | Dòng `[STAT]`. |
| High-water mark | Mức cao nhất từng đạt | Peak queue usage. |
| Integer division | Phép chia số nguyên | Có thể bỏ qua một số phần trăm. |
| Counter | Bộ đếm | Biến thống kê tăng theo sự kiện. |
| Confirmed success | Thành công đã xác nhận | Driver đã báo operation thành công. |
| Actual payload | Payload thực tế | Data đến từ SREC. |
| Padding excluded | Không tính padding | `0xFF` thêm vào không cộng bytes. |

### Công thức tiến độ erase

```text
Erase progress = erasedSectorCount × 100 / totalSectorCount
```

### Công thức bước báo cáo download

```text
currentStep = programmedDataBytes / DOWNLOAD_REPORT_STEP_BYTES
```

---

## 14. AppValidator và Vector Table

| English term | Tiếng Việt | Ý nghĩa |
|---|---|---|
| Vector table | Bảng vector ngắt | Bảng đầu Application image. |
| Initial MSP | Giá trị MSP ban đầu | Word đầu tiên của vector table. |
| Main Stack Pointer | Con trỏ ngăn xếp chính | MSP. |
| Stack top | Đỉnh ngăn xếp | Có thể bằng địa chỉ cuối SRAM. |
| Reset vector | Vector reset | Word thứ hai của vector table. |
| Reset handler | Hàm xử lý reset | Entry point của Application. |
| Handler address | Địa chỉ hàm xử lý | Reset Vector sau khi bỏ Thumb bit. |
| Thumb bit | Bit chế độ Thumb | Bit 0 của function address phải bằng 1. |
| Vector validation | Kiểm tra vector | Kiểm tra MSP và Reset Vector. |
| MSP alignment | Căn chỉnh MSP | MSP phải align phù hợp. |
| SRAM range | Phạm vi SRAM | Vùng MSP hợp lệ. |
| Application address | Địa chỉ Application | Handler phải thuộc Application region. |
| Diagnostic code | Mã chẩn đoán | Lý do Application không hợp lệ. |
| Invalid MSP | MSP không hợp lệ | Không thuộc SRAM hoặc sai alignment. |
| Invalid Thumb bit | Thumb bit không hợp lệ | Bit 0 của Reset Vector bằng 0. |
| Invalid Reset Handler | Reset Handler không hợp lệ | Handler ngoài Application region. |
| Erased vector table | Vector table đã bị xóa | Hai word thường là `0xFFFFFFFF`. |
| Volatile read | Phép đọc volatile | Ép compiler thực hiện đọc Flash thật. |

### Mã chẩn đoán

| Identifier | Tiếng Việt |
|---|---|
| `APP_DIAG_NONE` | Không có lỗi. |
| `APP_DIAG_INVALID_MSP` | MSP không hợp lệ. |
| `APP_DIAG_INVALID_THUMB_BIT` | Thumb bit không hợp lệ. |
| `APP_DIAG_INVALID_RESET_HANDLER` | Reset Handler không hợp lệ. |

---

## 15. JumpManager và ARM Cortex-M4

| English term | Tiếng Việt | Ý nghĩa |
|---|---|---|
| JumpManager | Bộ quản lý chuyển Application | Chuẩn bị CPU và branch sang app. |
| Vector Table Offset Register | Thanh ghi địa chỉ bảng vector | VTOR. |
| VTOR relocation | Chuyển vị trí bảng vector | Đặt VTOR tới `0x00020000`. |
| SysTick | Bộ định thời hệ thống | Được dừng trước khi jump. |
| NVIC | Bộ điều khiển ngắt lồng nhau | Quản lý external interrupt. |
| Disable IRQ | Vô hiệu hóa yêu cầu ngắt | Ngăn ISR Bootloader chạy trong app. |
| Clear pending interrupt | Xóa ngắt đang chờ | Xóa trạng thái pending NVIC. |
| Branch | Nhảy tới địa chỉ lệnh | Gọi Application Reset Handler. |
| Function pointer | Con trỏ hàm | Dùng để gọi Reset Handler. |
| Reset-like state | Trạng thái gần giống reset | IRQ/SysTick/NVIC được dọn trước hand-off. |
| Deinitialize | Hủy khởi tạo | Dừng ảnh hưởng của peripheral Bootloader. |
| Peripheral | Ngoại vi | UART, GPIO, FTFC, clock. |
| Cortex-M4 | Lõi ARM Cortex-M4 | CPU của S32K144. |
| System Control Space | Không gian điều khiển hệ thống | Vùng địa chỉ SCB, NVIC, SysTick. |
| Architecturally fixed address | Địa chỉ cố định theo kiến trúc | Địa chỉ ARM chuẩn, không phụ thuộc SDK. |

---

## 16. Clock và Watchdog

| English term | Tiếng Việt | Ý nghĩa |
|---|---|---|
| Clock source | Nguồn clock | Nguồn xung cho CPU/peripheral. |
| FIRC | Dao động RC nhanh nội | 48 MHz, cấp clock cho UART. |
| FIRCDIV2 | Clock FIRC qua bộ chia | Nguồn bất đồng bộ được PCC chọn. |
| SOSC | Dao động hệ thống ngoài | Crystal 8 MHz. |
| SPLL | Vòng khóa pha hệ thống | Tạo clock 96 MHz trước chia. |
| Core clock | Clock lõi CPU | 48 MHz trong thiết kế. |
| Bus clock | Clock bus | 24 MHz. |
| Slow clock | Clock chậm | 24 MHz. |
| Clock divider | Bộ chia clock | Giảm tần số nguồn. |
| Clock validity | Tính hợp lệ clock | Cờ nguồn clock đã ổn định. |
| Clock switch | Chuyển nguồn clock | Chọn SPLL làm RUN clock. |
| Clock timeout | Hết thời gian chờ clock | Nguồn clock không đạt valid. |
| Watchdog | Bộ giám sát | Reset MCU nếu không được refresh/disable. |
| Watchdog timeout | Watchdog hết thời gian | Gây reset. |
| Unlock sequence | Chuỗi mở khóa | Cho phép thay đổi watchdog. |
| CMD32EN | Cho phép lệnh watchdog 32-bit | Cần giữ bằng 1. |
| UPDATE bit | Bit cho phép cập nhật | Cho phép cấu hình lại watchdog. |
| EN bit | Bit bật watchdog | Xóa để disable. |
| Read-modify-write | Đọc - sửa - ghi | Chỉ thay đổi bit cần thiết. |
| Blind write | Ghi đè toàn bộ | Có thể vô tình xóa CMD32EN. |
| Reset loop | Vòng lặp reset | Jump app rồi watchdog reset liên tục. |
| Lockup | Trạng thái khóa CPU | Fault nghiêm trọng gây reset. |

---

## 17. Công cụ phía PC

| English term | Tiếng Việt | Chức năng |
|---|---|---|
| Loader | Công cụ nạp | Gửi menu command và SREC. |
| Monitor | Công cụ giám sát | Nghe log UART. |
| Catch board | Giữ board trong Bootloader | Gửi CR trong entry window. |
| Baud scanner | Công cụ quét baud | Tìm baud giải mã ổn định. |
| Offline validation | Kiểm tra ngoại tuyến | Kiểm tra SREC trước khi gửi board. |
| Command-line argument | Tham số dòng lệnh | Ví dụ COM13, file SREC, `--catch`. |
| `--catch` | Tùy chọn giữ Bootloader | Gửi CR trong cửa sổ UART. |
| `--skip-erase` | Tùy chọn bỏ qua erase | Dùng khi vùng app đã erase. |
| Serial monitor | Trình theo dõi cổng nối tiếp | `uart_monitor.py`. |
| Download complete | Tải hoàn tất | Board đạt FINISHED. |
| Timeout | Hết thời gian chờ | Không nhận được chuỗi mong đợi. |
| Validation report | Báo cáo kiểm tra | Record count và address range. |
| Streaming delay | Độ trễ truyền từng dòng | Giảm nguy cơ RX overflow. |
| Fault injection | Tiêm lỗi có chủ đích | Tạo SREC sai để kiểm tra error policy. |
| Happy path | Luồng thành công chuẩn | Erase, download, validate và jump thành công. |
| Test scenario | Kịch bản kiểm thử | Chuỗi thao tác và kết quả mong đợi. |
| Integration test | Kiểm thử tích hợp | Kiểm tra toàn bộ hệ thống trên board. |
| Unit test | Kiểm thử đơn vị | Kiểm tra từng module độc lập. |
| Mock | Đối tượng mô phỏng | Thay FlashDriver thật khi unit test. |

---

## 18. Thuật ngữ thiết kế và tài liệu

| English term | Tiếng Việt |
|---|---|
| Architecture | Kiến trúc |
| Architecture document | Tài liệu kiến trúc |
| Design | Thiết kế |
| Detailed design | Thiết kế chi tiết |
| Component | Thành phần |
| Component diagram | Sơ đồ thành phần |
| Sequence diagram | Sơ đồ tuần tự |
| State diagram | Sơ đồ trạng thái |
| Flowchart | Lưu đồ |
| File structure | Cấu trúc file/thư mục |
| Interface | Giao diện lập trình |
| Public API | API công khai |
| Private helper | Hàm hỗ trợ nội bộ |
| Dependency | Quan hệ phụ thuộc |
| Coupling | Mức độ phụ thuộc giữa module |
| Cohesion | Tính tập trung trách nhiệm |
| Separation of concerns | Phân tách trách nhiệm |
| Single responsibility | Nguyên tắc đơn trách nhiệm |
| Abstraction | Sự trừu tượng hóa |
| Encapsulation | Sự đóng gói |
| Data ownership | Quyền sở hữu dữ liệu |
| Contract | Cam kết giao diện |
| Precondition | Điều kiện trước |
| Postcondition | Điều kiện sau |
| Invariant | Điều kiện bất biến |
| Implementation | Phần hiện thực |
| Configuration | Cấu hình |
| Compile-time check | Kiểm tra tại thời điểm biên dịch |
| Runtime check | Kiểm tra khi chương trình chạy |
| Defensive validation | Kiểm tra phòng vệ |
| Defense in depth | Phòng vệ nhiều lớp |
| Traceability | Khả năng truy vết |
| Maintainability | Khả năng bảo trì |
| Testability | Khả năng kiểm thử |
| Reliability | Độ tin cậy |
| Portability | Khả năng di chuyển nền tảng |
| Scalability | Khả năng mở rộng |
| Limitation | Hạn chế |
| Constraint | Ràng buộc |
| Deviation | Điểm sai khác có chủ đích |
| Trade-off | Sự đánh đổi |
| Rationale | Lý do thiết kế |
| Root-cause analysis | Phân tích nguyên nhân gốc |
| Lessons learned | Bài học kinh nghiệm |
| Verification | Xác minh |
| Validation | Thẩm định tính đúng |
| Acceptance criteria | Tiêu chí chấp nhận |
| Expected result | Kết quả mong đợi |
| Actual result | Kết quả thực tế |
| Evidence | Bằng chứng |
| Requirement compliance | Mức độ đáp ứng yêu cầu |

---

## 19. Build và Debug

| English term | Tiếng Việt |
|---|---|
| Build | Biên dịch và liên kết |
| Clean build | Xóa kết quả cũ và build lại |
| Compiler | Trình biên dịch |
| Linker | Trình liên kết |
| Toolchain | Bộ công cụ biên dịch |
| Build configuration | Cấu hình build |
| Debug configuration | Cấu hình debug/nạp |
| Debugger | Trình gỡ lỗi |
| Program | Nạp chương trình vào Flash |
| Verify | Xác minh dữ liệu đã nạp |
| Terminate | Kết thúc phiên debug |
| Resume | Tiếp tục chạy CPU trong phiên debug |
| Halt | Dừng CPU |
| Breakpoint | Điểm dừng |
| Watch expression | Biểu thức theo dõi |
| Build warning | Cảnh báo biên dịch |
| Build error | Lỗi biên dịch |
| Link error | Lỗi liên kết |
| Undefined reference | Tham chiếu chưa được định nghĩa |
| Implicit declaration | Khai báo hàm ngầm |
| Unused function | Hàm không được sử dụng |
| Binary | File nhị phân |
| ELF file | File thực thi và liên kết |
| SREC file | File Motorola S-record |
| Debug symbol | Ký hiệu debug |
| Source file | File mã nguồn |
| Header file | File khai báo |
| Object file | File đối tượng sau biên dịch |
| Build artifact | Sản phẩm của quá trình build |
| Flash image | Ảnh chương trình Flash |

---

## 20. Từ viết tắt quan trọng

| Viết tắt | English đầy đủ | Tiếng Việt |
|---|---|---|
| MCU | Microcontroller Unit | Vi điều khiển |
| CPU | Central Processing Unit | Bộ xử lý trung tâm |
| UART | Universal Asynchronous Receiver/Transmitter | Bộ thu phát nối tiếp không đồng bộ |
| LPUART | Low-Power UART | UART công suất thấp |
| ISR | Interrupt Service Routine | Trình phục vụ ngắt |
| GPIO | General-Purpose Input/Output | Vào/ra đa dụng |
| FTFC | Flash Memory Module / Flash Controller | Bộ điều khiển Flash |
| SREC | Motorola S-record | Định dạng bản ghi Motorola |
| MSP | Main Stack Pointer | Con trỏ ngăn xếp chính |
| VTOR | Vector Table Offset Register | Thanh ghi địa chỉ bảng vector |
| NVIC | Nested Vectored Interrupt Controller | Bộ điều khiển ngắt vector lồng nhau |
| SCB | System Control Block | Khối điều khiển hệ thống |
| PCC | Peripheral Clock Control | Điều khiển clock ngoại vi |
| SCG | System Clock Generator | Bộ tạo clock hệ thống |
| FIRC | Fast Internal Reference Clock | Clock tham chiếu nội nhanh |
| SOSC | System Oscillator | Dao động hệ thống ngoài |
| SPLL | System Phase-Locked Loop | Vòng khóa pha hệ thống |
| WDOG | Watchdog | Bộ giám sát |
| SRAM | Static Random-Access Memory | Bộ nhớ RAM tĩnh |
| P-Flash | Program Flash | Flash chương trình |
| RWW | Read While Write | Đọc trong khi ghi |
| FIFO | First In, First Out | Vào trước ra trước |
| API | Application Programming Interface | Giao diện lập trình ứng dụng |
| SDK | Software Development Kit | Bộ công cụ phát triển phần mềm |
| IDE | Integrated Development Environment | Môi trường phát triển tích hợp |
| CRC | Cyclic Redundancy Check | Kiểm tra dư thừa tuần hoàn |
| CR | Carriage Return | Ký tự về đầu dòng |
| LF | Line Feed | Ký tự xuống dòng |
| IRQ | Interrupt Request | Yêu cầu ngắt |
| DSB | Data Synchronization Barrier | Hàng rào đồng bộ dữ liệu |
| ISB | Instruction Synchronization Barrier | Hàng rào đồng bộ lệnh |
| RAM | Random-Access Memory | Bộ nhớ truy cập ngẫu nhiên |
| ROM | Read-Only Memory | Bộ nhớ chỉ đọc |

---

## 21. Cụm từ thường dùng khi thuyết trình

| English | Cách diễn đạt tiếng Việt tự nhiên |
|---|---|
| The bootloader validates the application | Bootloader kiểm tra tính hợp lệ của Application. |
| The button is active-high | Nút nhấn tác động ở mức logic cao. |
| The queue owns a copy of the payload | Hàng đợi sở hữu một bản sao độc lập của payload. |
| The parser rejects the record | Bộ parser từ chối bản ghi. |
| The engine consumes the job | FlashEngine xử lý và tiêu thụ job. |
| The phrase is partially filled | Phrase mới được điền một phần. |
| The phrase is padded with `0xFF` | Phần còn thiếu của phrase được chèn `0xFF`. |
| The address is out of range | Địa chỉ nằm ngoài phạm vi cho phép. |
| The address is not aligned | Địa chỉ chưa được căn chỉnh. |
| The record overlaps previous data | Bản ghi chồng lấn dữ liệu trước đó. |
| The flash command is launched | Lệnh Flash được khởi chạy. |
| The driver waits for command completion | Driver chờ lệnh hoàn tất. |
| The function executes from SRAM | Hàm được thực thi từ SRAM. |
| Interrupts are temporarily disabled | Các ngắt được tạm thời vô hiệu hóa. |
| The first error is latched | Lỗi đầu tiên được chốt và lưu giữ. |
| The state transition is rejected | Chuyển trạng thái bị từ chối. |
| The final phrase is flushed | Phrase cuối được hoàn thiện và ghi xuống Flash. |
| Progress is updated after success | Tiến độ chỉ được cập nhật sau khi thao tác thành công. |
| Padding bytes are excluded | Các byte padding không được tính. |
| The application takes control | Application tiếp nhận quyền điều khiển CPU. |
| The board remains in the bootloader | Board tiếp tục ở chế độ Bootloader. |
| The host streams the SREC file | Máy tính truyền tuần tự file SREC. |
| The application region is erased | Vùng Application được xóa. |
| The firmware update completed successfully | Quá trình cập nhật firmware đã hoàn tất thành công. |
| The design uses defense in depth | Thiết kế sử dụng cơ chế bảo vệ nhiều lớp. |
| The behavior was verified on real hardware | Hành vi đã được xác minh trên phần cứng thực. |

---

## Ghi chú cuối

Các identifier nên giữ nguyên trong source code:

```text
Bootloader, Application, FlashEngine, FlashDriver, FlashJobQueue,
SrecParser, LineBuilder, InputFilter, RxRingBuffer, BootStateMachine,
BootStatistic, ErrorHandler, AppValidator, JumpManager, BootButton,
ClockDriver, GpioDriver, UartDriver, Scheduler.
```

Các state và macro nên giữ nguyên:

```text
BOOT_STATE_MENU
BOOT_STATE_ERASING_APP
BOOT_STATE_RECEIVING
BOOT_STATE_FLUSHING
BOOT_STATE_FINISHED
BOOT_STATE_ERROR

FLS_SREC_JOB_PROGRESSING
FLS_SREC_JOB_DONE
FLS_JOB_BUSY
FLS_JOB_DONE

FLASH_JOB_ERASE_APP
FLASH_JOB_PROGRAM_DATA
FLASH_JOB_END_DOWNLOAD
```

> **Tóm tắt dự án:** SW2 là bộ chọn mode chính; UART entry window là cơ chế recovery; SREC được kiểm tra rồi chuyển thành Flash Job; FlashEngine quản lý phrase 8 byte, gap và padding; FlashDriver thực hiện lệnh FTFC từ SRAM; Application chỉ được chạy sau khi vector table hợp lệ.
