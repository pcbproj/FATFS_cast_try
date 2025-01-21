/*******************************************************************/
#include "main.h"



/*******************************************************************/
#define BUFFER_SIZE		(uint16_t)512	// read write one sector size in bytes 




uint32_t writeBuffer[BUFFER_SIZE]; 
uint32_t readBuffer[BUFFER_SIZE];


uint8_t readData_8[BUFFER_SIZE];

uint8_t writeBuffer_bytes[BUFFER_SIZE];

FATFS fs;
FRESULT res;
FIL file;
FILINFO file_info;
DIR dir;

SD_CardInfo SDCardInfo;



FRESULT SD_CardMount(void){
    res = f_mount(&fs, "/", 1);	// примонтировать раздел немедленно 
    
	if(res != FR_OK) {
        printf("--- f_mount() failed, res = %d\r\n", res);
        return FR_INT_ERR;
    }
    printf("+++ f_mount() done!\r\n");
	return FR_OK;
}





FRESULT SD_CardFileRead(void){
	
	const char file_name[12] = "fstest00.txt"; 

	uint8_t readed_data[128];
	unsigned int BytesToRead = 8;
	unsigned int BytesReaded = 0;
 
	//res = f_opendir(&dir, "/");
	//if(res != FR_OK){
	//	printf("--- Opening Root directory FAILED. ErrorCode = %d \n" , res);
	//	return res;
	//}
	
	printf("--- Checking for existed file %s on SD-card \n", file_name);
	res = f_stat(file_name, &file_info);
	if (res != FR_OK){
		printf("--- File %s does not exist on SD-card \n" , file_name);
		return res;
	}
	printf("+++ FOUND file %s \n", file_info.fname);
	printf("+++ file SIZE = %d bytes \n", (uint32_t)file_info.fsize);
	printf("--- opening TXT-file %s ... \n", file_info.fname); //NOTE: <<-- here use file_info.fname from f_stat() instead file_name

    res = f_open(&file, file_info.fname, FA_READ);  
    
	if(res == FR_OK){ 
		printf("+++ opening file complete sucessfully. ErrorCode res = %d \n", res);
		
		printf("--- Starting file reading... \n");

		//res = f_read(&file, readData_8, (uint16_t)file_info.fsize, &BytesReaded);	// read whole file data. file must be less than array readData_8[] 
		
		// проверка сколько секторов нужно нам читать, чтобы весь текст из файла вычитать.
		
		// сделать ограничение по 2048 байт = 4 сектора, чтобы сильно длинные файлы читались по частям и 
		// отправлялись по UART1.
		 
		// создать нужный по размеру массив для хранения данных
		// считать несколько секторов, чтобы весь текст файла считался.


		res = f_read(&file, readData_8, (uint16_t)BUFFER_SIZE, &BytesReaded);	// read one sector. BUFFER_SIZE = sector_size = 512 bytes 
		if(res == FR_OK){
			printf("+++ File reading successfully! Readed string: \n");
			usart1_send(readData_8, BytesReaded);
			printf("\n--- Readed bytes number = %d \n" , BytesReaded );
			f_close(&file);
		}
		else{
			printf("--- reading file FAILED! ErrorCode = %d \n", res);  // return error code = 9 (FR_INVALID_OBJECT)
			f_close(&file);
			return res;
		}
	}	
	else{
		printf("--- Error opening file. ErrorCode res = %d \n", res);
    }
	
	return res;
}




FRESULT SD_CardCreateFile(void){
	const char fl_name[12] = "crt0.txt";
	uint16_t WritedBytes = 0;
	
    res = f_open(&file, fl_name, FA_CREATE_ALWAYS|FA_READ|FA_WRITE);

	if (res != FR_OK) printf("--- Creating file %s FAILED! Error code = %d \n", fl_name, res);
	else{
		printf("+++ file %s was created successfully \n", fl_name);
		printf("+++ Writing test string into file %s ... \n", fl_name);

		uint8_t *file_text =  malloc(128 * sizeof(uint8_t));	// reserve memory for string file_text
		sprintf(file_text, "Test string for file creation!!! \n");	// write test string into string file_text
		res = f_write(&file, file_text, strlen(file_text), &WritedBytes); // write string file_text into file

		if (res != FR_OK) printf("--- Writing into file %s FAILED! Error code = %d \n", fl_name, res);
		else printf("+++ Writing string into file %s successfully \n", fl_name);
		f_close(&file);
	}
	return res;
}






/*******************************************************************/
int main(){

	SD_Error SD_ErrorState = SD_OK;	// SD SDIO error status
	
	RCC_Init();
	APP_GPIO_Init();
	USART1_Init();		// for debug baudrate = 115200

	
	printf("--- System started! ---- \n");
	
	GPIOE->ODR |= (1<<13)|(1<<14)|(1<<15);  

	printf("--- SD-card initialization started! ---- \n");
    // Иницилизация карты
	SD_ErrorState = SD_Init();

	if (SD_ErrorState == SD_OK) {
	
		printf("--- SD-card Getting information! ---- \n");
    	// Получаем информацию о карте
    	SD_GetCardInfo(&SDCardInfo);
    	
		printf("--- SD-card Selection! ---- \n");
    	// Выбор карты и настройка режима работы
    	SD_SelectDeselect((uint32_t) (SDCardInfo.RCA << 16));
    	SD_SetDeviceMode(SD_POLLING_MODE);

		
		res = SD_CardMount();

		// чтение файла с карты памяти
		if (res == FR_OK){
			res = SD_CardFileRead();
		}
		else{
			printf("--- SD-card mounting failed... \n");
		}

		// создание нового файла на карте и запись в него тестовых данных
		if(res == FR_OK) {
			res = SD_CardCreateFile();
		}
		else{
			printf("--- ERROR reading file on SD-card \n");
			printf("--- New file was NOT CREATED on SD-card \n");


		}
	}
	else{
		printf("--- SD-card not found!! ---- \n");
	}

	
	while(1)
	{
	}

	return 0;
}



/*******************************************************************/

	