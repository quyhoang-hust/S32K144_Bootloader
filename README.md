# S32K144 Mini Bootloader Architecture

## 1. Tổng quan kiến trúc

### Mini Bootloader có nhiệm vụ:

* **1.** Khởi tạo phần cứng cần thiết

* **2.** Xác định chế độ khởi động từ SW2 

* **3.** Cho phép host giữ Bootloader qua UART như một cơ chế recovery

* **4.** Kiểm tra tính hợp lệ của application

* **5.** Cung cấp menu erase và download

* **6.** Nhận file Srec qua UART

* **7.** Phân tích Srec Record     

* **8.** Chuyển dữ liệu thành flash job

* **9.** Gom dữ liệu thành phrase 8 byte

* **10.** Erase và program vùng Application Flash

* **11.** Kiểm tra lại Application và chuyển quyền điều khiển sang Application   

### Memory Layout

<img width="548" height="452" alt="Image" src="https://github.com/user-attachments/assets/65e20a92-eef7-4ec1-981a-73458870248d" />


* Các vùng chính:

	* BOOTLOADER_START_ADDRESS = 0x0000 0000
	*  APPLICATION_START_ADDRESS = 0x0002 0000
	*  APPLICATION_END_ADDRESS = 0x0008 0000
	
* Kích thước vùng Application

	* 0x0008 0000   - 0x0002 0000
	 = 0x0006 0000
	 = 384 KB

* Với sector 4 KB:
	
	*384 KB / 4 KB = 96 Sector 

## 2. Component Diagram

### Sơ đồ thành phần tổng thể

<img width="600" height="551" alt="Image" src="https://github.com/user-attachments/assets/d006dbaf-180f-4681-8abc-3aefcf66a1a0" />

* Bootstrap Layer

	<img width="786" height="621" alt="Image" src="https://github.com/user-attachments/assets/573938c6-17af-4ed4-9605-d18ba6aae8c6" />

* Scheduler Layer

	<img width="530" height="658" alt="Image" src="https://github.com/user-attachments/assets/c3abc9ca-3241-4b4c-a629-8ec20b2ed5ef" />

* UART  Pipeline

	<img width="265" height="452" alt="Image" src="https://github.com/user-attachments/assets/80f28d93-2001-48e2-8443-44ccb546e126" />

* Flash Pipeline

	<img width="438" height="621" alt="Image" src="https://github.com/user-attachments/assets/2480c7b5-fc1a-44bc-9c17-47908d50f994" />

### Quan hệ giữa các module

<img width="533" height="471" alt="Image" src="https://github.com/user-attachments/assets/37970df7-05eb-485b-bef4-cd07fbef3704" />

### Luồng dữ liệu SREC

<img width="563" height="698" alt="Image" src="https://github.com/user-attachments/assets/76e740f9-ae0d-4ee4-a0e9-182b146e9df7" />

* Nguyên tắc phân lớp

	* UART ISR chỉ nhận byte và đưa vào RingBuffer
	* Task SREC chỉ phân tích record và tạo job
	* FlashEngine là module duy nhất xử lý phrase, gap, padding và thứ tự flash
	* FlashDriver là lớp thấp nhấy trực tiếp giao tiếp với FTFC
	* Task không được gọi trực tiếp FTFC
	* Parser không được program flash
	* FlashDriver không được hiểu SREC 

## 3. Trách nhiệm của các Task

### Scheduler sử dụng mô hình cyclic executive

* Không có RTOS và không có dynamic scheduling
* Mỗi Task được gọi tuần tự trong mỗi vòng scheduler

		for(;;){
			ErrorHandler_ProcessingPending();
			Task_MenuProcess();
			Task_SrecRxProcess();
			Task_FlashProgram();
			Task_ReportStatistic();
		}

### Task_MenuProcess

* Trạng thái hoạt động

			BOOT_STATE_MENU

* Trách nhiệm
	
	* Hiển thị menu bootloader
	* Đọc menu command từ RxRingBuffer
	* Phân loại lựa chọn người dùng
	* Option 1 tạo erase job
	* Option 2 khởi tạo phiên download
	* Lệnh không hợp lệ chỉ báo lỗi menu không tạo fatal error
	
* Menu

	* 1. Erase Application Region
	* 2. Start Download Program

* Option 1:

	* Khi nhận 1:
	
		* Push FLASH_JOB_ERASE_APP -> Set BOOT_STATE_ERASING_APP
		
		* Task không tự erase flash mà chỉ tạo yêu cầu cho FlashEngine

* Option 2:

	* Khi nhận 2, task phải chuẩn bị một phiên tải mới
	
		<img width="427" height="707" alt="Image" src="https://github.com/user-attachments/assets/d1a0e2b7-9e24-4ab8-908b-d14f45fd01ef" />

	* Việc disable RX Interrupt trong giai đoạn reset context ngăn byte mới đi vào buffer khi parser và LineBuilder chưa sẵn sàng
	
* Những việc Task_MenuProcess không được làm

	* Không erase flash trực tiếp
	* Không program flash trực tiếp
	* Không parser SREC
	* Không xử lý phrase
	* Không padding dữ liệu
	* Không gọi FTFC  

### Task_SrecRxProcess

* Trạng thái hoạt động

			BOOT_STATE_RECEIVING

* Trách nhiệm

	* Lấy byte từ RxRingBuffer
	* Giới hạn số byte xử lý trong một scheduler cycle
	* Kiểm tra byte bằng InputFilter
	* Ghép byte thành một dòng bằng LineBuilder
	* Parse dòng bằng SrecParser
	* Tạo flash job theo loại record
	* Chuyển sang FLUSING khi nhận end record

* Giới hạn mỗi cycle

		#define SREC_RX_MAX_BYTES_PER_CYCLE     (16u)

	* Mục đích 

		* Không để SREC task chiếm toàn bộ CPU
		* kHÔNG LÀM Flash Task bị starvation
		* Giữ scheduler có tính cyclic
		* Cho phép statistic và error handling tiếp tục chạy

* Xử lý RECORD

	* Data record
			
			S1,S2,S3

		* Task tạo:

			FLASH_JOB_PROGRAM_DATA

		* Job chứa:

			address
			length
			copy of payload bytes

	* End record
	
			S7. S8. S9

		* Task thực hiện

					Push FLASH_JOB_END_DOWNLOAD  

					|  

					Set BOOT_STATE_FLUSHING  

					|  

					Disable UART RX interrupt  

					|  

					Clear stale RX data

			* End job phải được push trước khi state chuyển FLUSHING để đảm bảo FlashEngine xử lý toàn bộ program job trước end marker


	* Infomation record
			
			S0,S5,S6

		* Các record này:
		
			* Vẫn được kiểm tra length
			* Vẫn được kiểm tra hex
			* Vẫn được kiểm tra bytecount
			* Vẫn được kiểm tra checksum
			* Không tạo flash job
			* Được bỏ qua sau khi validation thành công
			
	* Unsupported record
	
			S4

		* S4 là reserved type và bị xử lý như lỗi SREC

* Những việc Task_SrecRxProcess không được làm

	* Không gom phrase 8 byte
	* Không phát hiển gap ở tầng flash
	* Không padding 0xFF
	* Không gọi FlashDriver
	* Không thay đổi trực tiếp statistic của FlashEngine
	* Không giữ con trỏ tới buffer parser sau khi record kết thúc   

 
### Task_FlashProgram

* Trách nhiệm

		void Task_FlashProgram(void){
			FlashEngine_MainFunction();
		}

	* Toàn bộ business logic flash nằm trong FlashEngine
	
	* Task chịu trách nhiệm kích hoạt cyclic processing cho:
		
		* Erase Sequencing
		* Queue job acquisition
		* SREC job consumption
		* Phrase  assembly
		* Gap handling
		* Padding
		* Phrase programming
		* End-download flushing
		* Flash statistic update

* Task đơn giản là vì:

	* Tránh trùng logic giữa task và FlashEngine
	* FlashEngine có thể là unit test độc lập
	* Scheduler chỉ biết gọi module, không cần biết nội bộ flash

### Task_ReportStatistic

* Trạng thái hoạt động

	* Task chạy trong mọi state nhưng chỉ report khi điều kiện tương ứng xảy ra
	
* Trách nhiệm

	* Report fatal error
	* Report thay đổi bootloader state
	* Report erase progress
	* Report Download progress
	* Report thống kê cuối phiên
	* Tùy chọn report trạng thái nội bộ FlashEngine

* Thứ tự report

	* Fatal Error -> Bootloader state -> Erase progress -> Download progress -> FlashEngine trace
	
	* Fatal được ưu tiên để nguyên nhân xuất hiện trước hiệu ứng state transition
	
* VD Output:

			[BOOT] state=ERASING_APP
			[ERASE] progress=50% sectors=48/96
			[DOWNLOAD] bytes written=2048
			[STAT] lines=230 data_rec=228 end_rec=1 phrases=451 queue_peak=3/8

* Nguyên tắc:

	* Task chỉ đọc snapshot

		FlashEngine_GetStatisticSnapshot();
		BootStatistic_GetSnapshot();
		ErrorHandler_GetSnapshot();

	* Task không được thay đổi boot flow hoặc flash operation

##  	4. Flash Job Queue

### Mục đích

* FlashJobQueue tách rời tốc độ sản xuất job của SREC task và tốc độ tiêu thụ job của FlashEngine

		Task_SrecRxProcess -> producer
		FlashEngine -> consumer

### Các loại job


			typedef enum {
				FLASH_JOB_NONE = 0,
				FLASH_JOB_ERASE_APP,
				FLASH_JOB_PROGRAM_DATA,
				FLASH_JOB_END_DOWNLOAD
			} FlashJobType;

* **FLASH_JOB_ERASE_APP:**

	* Yêu cầu erase toàn bộ Application region
	* Job không cần payload

* **FLASH_JOB_PROGRAM_DATA:**

	* Chứa

			typedef struct 
			{
				uint32_t address;
				uint16_t length;
				uint8_t data[FLASH_JOB_MAX_SIZE];
			} FlashProgramDataType;

* **FLASH_JOB_END_DOWNLOAD:**

	* Đánh dấu tất cả SREC data record đã được nhận
	* FlashEngine dùng job này để:
		
		* Flush phrase cuối
		* Padding phần thiếu bằng 0xFF
		* Chờ Program thành công
		* Chuyển state sang FINISHED     	      

### Queue ownership

		memcpy(
			job.programData.data,
			data,
			length);

* Queue sở hữu bản copy riêng của payload

* Copy vì:

	* Buffer record ban đầu có thể nằm trong:
	
		* Local variable của task
		* SrecParser record
		* LineBuilder buffer
		* Stack frame

	* Sau khi task return hoặc parser xử lý record tiếp theo, dữ liệu đó có thể bị thay đổi
	
	* Nếu queue chỉ lưu pointer `const uint8_t * data` thì FlashEngine có thể đọc phải dữ liệu đã bị ghi đè
	  
	* Copy payload đảm bảo:
	
		* Job đã push
		
			-> dữ liệu thuộc quyền sở hữu queue
			-> parser có thể tiếp tục xử lý record mới       

### Cơ chế vòng FIFO

* Queue sử dụng:

		g_headIndex
		g_tailIndex
		g_jobCount

* Push

	* Copy job vào tail
	* Advance tail
	* Increment count
	
* Peek

	* Copy entry tại head ra current job
	
	* Peek không xóa entry
	
* Pop 

	* Clear entry tại head
	
	* Advance head
	
	* Decrement count
	
* Empty

	* g_jobCount == 0U
	
* Full

	* g_jobCount == FLASH_JOB_QUEUE_SIZE

### Overflow policy

* Khi queue đầy trả về False

		FlashJobQueue_PushProgramData(...)

* Task nâng:

		BOOT_ERROR_FLASH_JOB_QUEUE_OVERFLOW

	* Queue không được:
	
		* Ghi đè job cũ
		
		*  Drop job âm thầm
		
		* Thay đổi thứ tự FIFO
		
		* Tiếp tục download như chưa có lỗi  

* Mất một job có thể làm Application image sai nhưng checksum từng record vẫn đúng, nên overflow phải là fatal error

### Peak usage

* Queue theo dõi `g_peakUsage`khi push:

		g_jobCount++;
		if(g_jobCount > g_peakUsage)
		{             
			g_peakUsage = g_jobCount;
		}

* Cuối download có thể report

		queue_peak=3/8

* Ý nghĩa:

	* Queue capacity là 8
	
	* Mức sử dụng cao nhất quan sát được là 3
	
	* Còn headroom 5 slot trong phiên đó 

### FlashEngine Internal State Machine

* SREC Job State Machine

		typedef enum {
			FLS_SREC_JOB_DONE = 0,
			FLS_SREC_JOB_PROGRESSING 
		} FlsSrecJobStateType;

* FLS_SREC_JOB_PROGRESSING

	* Có nghĩa là FlashEngine đã acquire một PROGRAM_DATA job
	
	* Vẫn còn byte trong job chưa được consume
	
	* currentJobOffset < programData.length

* FLS_SREC_JOB_DONE

	* Có nghĩa là toàn bộ byte của current SREC job đã được consume
	
	* Queue entry có thể được pop
	
	* Không còn phụ thuộc vào payload của current job      
	
	* SREC job DONE không có nghĩa phrase cũng DONE
	
		* Một SREC record có thể kết thúc giữa phrase

				Phrase size = 8 byte
				Record cung cấp = 5 byte

				// Sau khi consume record

				SREC state = DONE
				Phrase state = BUSY

				// Phrase vẫn cần 3 byte từ record tiếp theo

### Flash Phrase Job State Machine


			typedef enum
			{
				FLS_JOB_DONE = 0,
				FLS_JOB_BUSY
			} FlsPhraseJobStateType;

*   FLS_JOB_DONE

	* Là không có active phrase
	* Phrase trước đã program thành công
	* Statistic của phrase trước đã cập nhật
	* FlashEngine có thể bắt đầu phrase mới

* FLS_JOB_BUSY

	* Phrase vừa được mở
	* Đang nhận data byte
	* Đang chờ record tiếp theo
	* Đang padding
	* Đã đủ 8 byte
	* Đang được submit cho FlashDriver
	* Chưa được xác nhận hoàn tất

### Quan hệ 2 state machine

* Các tổ hợp hợp lệ:
	
	* SREC_PROGRESSING + PHRASE_BUSY
	* SREC_DONE  + PHRASE_BUSY
	* SREC_DONE  + PHRASE_DONE

* Tổ hợp quan trọng nhất xảy ra khi SREC record đã hết nhưng phrase chưa đủ 8 byte

	* FLS_SREC_JOB_DONE + FLS_JOB_BUSY  

* VD:

	* Phrase base = 0x0002 09F0
	* Record cung cấp 4 byte tại 0x0002 09F0
	* Sau record:
		* nextFillOffset  = 4
		* actualDataByteCount = 4
		* SREC state = DONE
		* Phrase state = BUSY  
	* Record tiếp theo bắt đầu tại 0x0002 09F4 có thể fill tiếp phrase đó

### Active phrase context


			typedef struct
			{
				uint32_t baseAddress;

				uint8_t data[FLASH_PHRASE_SIZE];

				uint8_t nextFillOffset;

				uint8_t actualDataByteCount;

				bool isSubmitted;
			} FlashActivePhraseType;

* baseAddress:

	* Địa chỉ đã align xuống boundary 8 byte

			first address = 0x020003 
			phrase base = 0x020000

* data:

	* Buffer 8 byte được init bằng:

		FF FF FF FF FF FF FF FF
		
*  nextFillOffset:

	* Vị trí tiếp theo sẽ được ghi
	
* actualDataByteCount

	* Số byte thực sự đến từ SREC
	* Không bao gồm padding 0xFF

* isSubmitted

	* Cho biết phrase đã được chuyển cho FlashDriver chưa
	* Sau khi submit, phrase data không được phép thay đổi

### Unaligned start

* Nếu record bắt đầu tại địa chỉ không align 8 byte:

		Record address = 0x020003
		Phrase base = 0x020000
		Leading pad = 3

* Phrase:

		Offset:  0  1  2  3  4  5  6  7
		Data:    FF FF FF D0 D1 D2 D3 D4  

* actualDataByteCount chỉ đếm:

		D0 D1 D2 D3 D4 = 5 byte

	* Không đếm 3 byte FF

### Gap handling

* Gap trong cùng phrase

		Expected next address = 0x020003
		New record address = 0x020006

	* FlashEngine padding rồi tiếp tục fill tại offset tương ứng

			0x020003 .. 0x020005 = 0xFF 

* Gap vượt phrase hiện tại

		Active phrase end = 0x020008
		New address = 0x020020

	* FlashEngine:
	
		* Padding phrase hiện tại tới đủ 8 byte
		* Program phrase hiện tại
		* Không program toàn bộ vùng gap bằng 0xFF
		* Mở phrase mới tại địa chỉ mới
		
	* Không cần ghi vùng gap vì Application region đã được erase trước và đã chứa 0xFF

* Overlap hoặc địa chỉ đi lùi

	* Nếu:
		
		newAddress < expectedNextAddress

	* FlashEngine nâng:
		
		BOOT_ERROR_ADDRESS_OVERLAP

	* Không được ghi chồng dữ liệu cũ

### End-download flushing

* Khi FlashEngine nhận:

		FLASH_JOB_END_DOWNLOAD

* Nếu còn active phrase chưa đầy:

		Data: D0 D1 D2 FF FF FF FF FF

* FlashEngine:

	* Padding đến đủ 8 byte
	* Program phrase cuối
	* Chỉ khi program thành công mới release end job
	* Chuyển engine về IDLE
	* Chuyển BootloaderState sang FINISHED

			RECEIVING  

			|  

			End record  

			v  

			FLUSHING  

			|  

			Program tail phrase  

			v  

			FINISHED

### Erase mode

* FlashEngine erase một sector mỗi scheduler cycle

				ERASE job  

				|  

				MODE_ERASE  

				|  

				Erase sector at nextEraseAddress  

				|  

				Increment erasedSectorCount  

				|  

				Advance nextEraseAddress  

				|  

				Repeat next scheduler cycle

* Sau sector cuối : `MODE_ERASE_COMPLETE`

	* Mode trung gian này giữ BootloaderState ở ERASING_APP thêm một cycle để Task_ReportStatistic nhìn thấy:

			96/96 = 100% 

	* Chu kỳ sau mới chuyển:

			ERASING_APP -> MENU

### BOOTLOADER State Machine

* Các trạng thái

			BOOT_STATE_MENU
			BOOT_STATE_ERASING_APP
			BOOT_STATE_RECEIVING
			BOOT_STATE_FLUSHING
			BOOT_STATE_FINISHED
			BOOT_STATE_ERROR

* Sơ đồ state machine

* State MENU

	* Cho phép:
	
		* Hiển thị menu
		* Nhận menu command
		* Tạo erase job
		* Bắt đầu download session
		
	* Không cho phép:
		
		* Parse SREC data như application payload
		* Program flash nếu không có job hợp lệ

* State ERASING_APP

	* Cho phép:
	
		* FlashEngine erase từng sector
		* Task_ReportStatistic report progress
		
	* UART RX input không được dùng làm menu hoặc SREC input trong state này
	
	* Khi erase đủ 96 sector:
	
			ERASING_APP -> MENU
			
* State RECEIVING

	* Cho phép:
	
		* UART ISR đưa byte vào RingBuffer
		* Task_SrecRxProcess xử lý SREC
		* FlashEngine consume PROGRAM_DATA job
		* FlashEngine program phrase

	* Khi nhận S7/S8/S9:

			RECEIVING -> FLUSHING

* State FLUSHING

	* Trong State này:
	
		* Không nhận thêm SREC
		* FlashEngine xử lý các job còn lại
		* Phrase cuối được padding
		* End-download job chỉ release sau khi phrase cuối thành công
		
	* Khi hoàn tất

			FLUSHING -> FINISHED

* State FINISHED

	* Đây là terminal state của một download session
	* Ứng dụng không tự động jump ngay trong cùng phiên download
	* Sau khi download complete:
		
		* Reset board
			
				-> Bootloader chạy lại
				-> AppValidator kiểm tra image
				-> Jump Application

* State ERROR

	* ERROR là terminal state cho đến reset
	* Trong ERROR:
		
		* Không tiếp tục erase
		* Không tiếp tục program
		* Không lấy thêm job
		* Không tiếp tục parse SREC
		* Task report vẫn có thể in fatal diagnostic
		* Chỉ reset mới đưa hệ thống về lại bootstrap flow

### Bootstrap mode selection

* Mode selection cuối cùng:

	* Giữ SW2 + RESET
			
			-> HIGH/HELD
			-> Bootloader

	* Không giữ SW2

			-> LOW / RELEASED
			-> Mở UART recovery window
			-> Nếu không có host request
			-> Validate application
			-> Jump nếu application hợp lệ

* UART entry là secondary recovery mechanism

		Primary entry = SW2
		Secondary entry = UART recovery
		Fallback entry = Invalid Application

### Flash Protection Design

* Vùng được phép ghi

	* Chỉ Application region:

			0x0002 0000 <= address < 0x0008 0000

	* Bootloader region không được phép erase/program
	
	* Flash Configuration Field nằm trong Bootloader region nên cũng được bảo vệ        

### Protection tại SREC/FlashEngine layer

* Trước khi xử lý program job:

		MemoryMap_IsApplicationRangeValid(
			address,
			length);

* Kiểm tra:

	* Start address nằm trong Application region
	* Toàn bộ range nằm trong Application region
	* Không overflow khi tính range
	* Record không overlap dữ liệu trước

* Trước khi program phrase kiểm tra toàn bộ phrase 8 byte nằm trong Application region

			MemoryMap_IsApplicationPhraseValid(
				phraseBase);

* Trước khi erase kiểm tra sector address align đúng, toàn sector nằm trong Application Region
			
			MemoryMap_IsApplicationSectorValid(
				sectorAddress);


* Protection tại FlashDriver layer

	* FlashDriver không tin hoàn toàn FlashEngine.

	* Driver validate lại 

		*  Erase:
		
			* sector alignment
			* application sector range
			
		* Program
		
			* data pointer khác NULL
			* phrase alignment
			* application phrase range   

* Command whitelist

	* FlashDriver chỉ cho phép 2 FTFC Command:
	

						PROGRAM_PHRASE = 0x07
						ERASE_SECTOR = 0x09

	* Không có API để gửi command FTFC tùy ý
	
	*  Giúp:
	
		* Mass erase ngoài ý muốn
		* Program Check
		* Security command
		* Command không được thiết kế trong bài

### Alignment protection

* Sector erase

	*  Địa chỉ phải align theo sector size:

			address % 4096 == 0
	

* Phrase program 

	* Địa chỉ phải align theo 8 byte

			address % 8 == 0

	* SREC record có thể bắt đầu unaligned, nhưng FlashEngine align xuống phrase base và dùng padding
	
	* FlashDriver chỉ nhận địa chỉ đã align 

### Read-While-Write protection

* S32K144 sử dụng P-Flash mà bootloader code và Application region cùng thuộc vùng flash thực thi

* Khi FTFC đang erase/program, không được tiếp tục fetch instruction từ vùng flash đang bị ảnh hưởng

* Áp dụng các giải pháp sau::

	* 1. Các hàm crtitical của FlashDriver chạy từ SRAM bằng RAM_FUNC
	
	* 2. Interrupt bị mask trong thời gian flash command
	
	* 3. FCCOB được load
	
	* 4. FTFC command được launch
	
	* 5. Driver poll CCIF từ SRAM tới khi xong
	
	* 6. Kết quả được latch
	
	* 7. PRIMASK được restore



		<img width="392" height="623" alt="Image" src="https://github.com/user-attachments/assets/2a8e4812-b8da-49cf-bd45-c658809dce8d" />   

* Tính  cyclic vẫn được giữ ở tầng FlashEngine

	* Một sector mỗi scheduler cycle
	* Một phrase operation mỗi scheduler cycle
	* Không drain toàn queue trong một lần gọi
	* Context được giữ qua các cycle

## 8. Fatal Error Policy

### Mục tiêu

* Ngừng mọi thay đổi flash sau lỗi nghiêm trọng

* Giữ nguyên lỗi đầu tiên

* Không để lỗi phụ che lỗi gốc

* Cho phép report nguyên nhân

* Chỉ reset mới khởi động lại phiên 
							  
### Các loại Fatal Error

* BOOT_ERROR_RX_OVERFLOW

* BOOT_ERROR_LINE_TOO_LONG

* BOOT_ERROR_INVALID_SREC

* BOOT_ERROR_ADDRESS_OUT_OF_RANGE

* BOOT_ERROR_ADDRESS_OVERLAP

* BOOT_ERROR_FLASH_JOB_QUEUE_OVERFLOW

* BOOT_ERROR_FLASH_ERASE_FAILED

* BOOT_ERROR_FLASH_WRITE_FAILED

* BOOT_ERROR_UNSUPPORTED_FLASH_COMMAND

* BOOT_ERROR_FLASH_DRIVER_ADDRESS_VIOLATION

* BOOT_ERROR_FLASH_DRIVER_ALIGNMENT_VIOLATION

* BOOT_ERROR_INTERNAL_STATE            

### First-error latch

* Chỉ lỗi đầu tiên được lưu:

	<img width="442" height="397" alt="Image" src="https://github.com/user-attachments/assets/1d894664-71b6-4e20-a4ac-6a9c23d56a94" /> 

* Nếu đã có fatal:

	* New secondary error -> không overwrite lỗi đầu tiên
	
	* VD:

			RX overflow
			-> parser nhận thiếu dòng
			-> checksum sai
			
			// Lỗi cần giữ là RX_OVERFLOW không phải checksum sai phát sinh sau đó

### Error từ task context

* Task có thể gọi trực tiếp:

		ErrorHandle_SetFatal(
			errorCode,
			address);

* Hàm:

	* Latch error đầu tiên
	
	* Chuyển BootloaderState sang ERROR

### Error từ ISR context

* ISR không nên thực hiện state transition phức tạp

* ISR dùng:

		ErrorHandler_SetFatalFromIsr();

	* Hàm chỉ publish pending error      

* Đầu mỗi scheduler cycle

		ErrorHandler_ProcessPending();

	* Chuyển pending ISR error thành fatal error đầy đủ trong task context

* Mô hình:

	<img width="425" height="595" alt="Image" src="https://github.com/user-attachments/assets/8e4704d7-c9aa-4c45-bd4f-f366a3c2cab1" />

### Hành vi trong ERROR

* Khi state là ERROR các module operational dừng xử lý

			if (BootStateMachine_IsError())
			{
				return;
			}

* Task_ReportStatistic vẫn report:

			[FATAL] code=... address=...
			[BOOT] state=ERROR

	* Không tự động trở về MENU vì điều đó có thể che mất lỗi và cho phép hoạt động tiếp trên context không còn đáng tin cậy

### Parser Diagnostic

* Mọi parser result có thể được đưa vào fatal address field

			1 = INVALID_ARGUMENT
			2 = INVALID_LENGTH
			3 = INVALID_FORMAT
			4 = INVALID_HEX
			5 = INVALID_BYTE_COUNT
			6 = INVALID_CHECKSUM
			7 = UNSUPPORTED_RECORD
			8 = INVALID_TERMINATION_PAIR
			9 = DATA_TOO_LONG

* VD:
			
			[SREC] parse_error=6 length=52 type=S0
			[FATAL] code=BOOT_ERROR_INVALID_SREC address=0x0000 0006

## Cách tính Progress

### Erase Progress

* FlashEngine lưu:

	* totalSectorCount
	* eraseSectorCoung

* Trong dự án: totalSectorCount = 96

* Công thức:

	* progress = (erasedSectorCount * 100u) / totalSectorCount;

* VD:

	* 48 / 96 = 50% 

### Tại sao có bước phần trăm bị bỏ qua

* Do integer division:

	* 23 / 96 x 100 = 23%
	
	*  24 / 96 x 100 = 25%  

* Nên log có thể chuyển: 23% -> 25% không có 24%

### Khi nào counter tăng ?

* Chỉ sau khi erase thành công

	* FlashDriver success -> erasedSectorCount++ -> nextEraseAddress += FLASH_SECTOR_SIZE

* Không tăm khi:

	* Job được tạo
	
	* Erase vừa được yêu cầu
	
	* FTFC chưa xác nhận hoàn tất
	
	* Driver trả lỗi

### Vì sao cần ERASE_COMPLETE mode ?

* Nếu sector cuối hoàn tất và state chuyển MENU ngay lập tức, report task không còn thấy state ERASING_APP

* Kết quả cũ:

		95/96 = 98%
		-> state chuyển MENU
		-> không report 96/96

* Thiết kế mới:

	* Cycle N:
	
		* Erase sector 96
		* erasedSectorCount = 96
		* engineMode = ERASE_COMPLETE
		* state vẫn ERASING_APP

	* Cùng cycle:
	
		* Task_ReportStatistic -> report 100% sectors=96/96

	* Cycle N+1:
	
		* engine chuyển IDLE
		
		* state chuyển MENU     

* Kết quả: 

			[ERASE] progress=100% sectors=96/96

### Download progress

* Flash Engine lưu

	* programmedDataBytes
	
	* programmedPhraseCount

* programmedDataBytes

	* Đếm số byte Application thực sự đến từ SREC và đã được commit thành công xuống flash
	
	* Không đếm:
	
		* Leading padding
		
		* Gap padding
		
		* End-of-download tail padding
		
		* Byte 0xFF tự thêm để đủ phrase

* programmedPhraseCount

	* Tăng một đơn vị cho mỗi phrase 8 byte program thành công

### Cập nhật statistic đúng thời điểm

* Statistic chỉ cập nhật khi phrase program thành công:

			g_statistic.programmedDataBytes += g_activePhrase.actualDataByteCount;
			g_statistic.programmedPhraseCount++;

* Không tăng statistic khi:

	* Record vừa parse
	
	* Job vừa push
	
	* Byte vừa copy vào phrase
	
	* Phrase vừa đủ 8 byte nhưng Driver chưa thành công
	
	* Program command thất bại           

### Ví dụ padding không được tính:

* Phrase chứa:

	* FF FF D0 D1 D2 D3 D4 FF

* Trong đó:

	* Padding = 3 byte
	
	* Data thật = 5 byte

* Sau program thành công không cộng 8

	* programmedPhraseCount += 1
	
	* programmedDataBytes += 5    	  

### Rate limiting

* Download progress được report theo bước

		#define DOWNLOAD_REPORT_STEP_BYTES (256u)

* Công thức:

		current = programmedDataBytes / DOWNLOAD_REPORT_STEP_BYTES;

* Chỉ report khi:

		currentStep > lastReportedStep

* Mục đích:

	* Không spam UART sau mỗi phrase
	
	* Giảm thời gian TX polling
	
	* Giảm ảnh hưởng tới SREC reception
	
	* Vẫn đủ thông tin theo dõi tiến độ

* Khi FINISHED, final count được report một lần bất kể có đúng mốc 256 hay không

		[DOWNLOAD] bytes_written=3608

### Thống kê cuối phiên

* Ví dụ

			[STAT] lines=230 data_rec=228 end_rec=1
						phrases=451 queue_peak=3/8

* Ý nghĩa:

	* lines: tổng số dòng SREC hoàn chỉnh đã nhận
	
	* data_rec: số S1/S2/S3 record
	
	* end_rec: số S7/S8/S9 record
	
	* phrases: số phrase 8 byte đã program thành công
	
	* queue_peak: số queue slot cao nhất từng sử dụng
	
	* /8: queue capacity

* Không nên suy ra tổng byte bằng phrases x 8, vì phrases có thể chứa padding 

* Tổng payload thực phải lấy từ: `programmedDataBytes`           
				
