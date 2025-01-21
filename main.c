/*******************************************************************/
#include "main.h"



/*******************************************************************/
#define BUFFER_SIZE					(uint16_t)512						// one sector size in bytes 
#define MAX_BYTES_TO_READ			(uint16_t)2048						// maximum array length for file data reading
#define MAX_NUM_SECTORS_TO_READ		(MAX_BYTES_TO_READ / BUFFER_SIZE)	// maximum sectors number for file data reading


uint8_t readData_8[BUFFER_SIZE];


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
	uint8_t readed_data[MAX_BYTES_TO_READ];
	unsigned int BytesReaded = 0;

 
	
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
		
		
		// проверка сколько секторов нужно считать, чтобы весь текст из файла вычитать.
		// ограничение по 2048 байт = 4 сектора

		// чтение всего файла целиком, т.к. размер файла меньше размера массива для считваемых данных
		if( ( (uint16_t)file_info.fsize ) < MAX_BYTES_TO_READ ){
			res = f_read(&file, readed_data, (uint16_t)file_info.fsize, &BytesReaded);	// read whole file data. file zise less than array length 
			
			if(res == FR_OK){
				printf("\n\n+++ File reading successfully! Readed string: \n");
				usart1_send(readed_data, BytesReaded);
				printf("\n\n--- Readed bytes number = %d \n" , BytesReaded );
				f_close(&file);
			}
		}
		else{
			
			uint16_t reading_iterations = (uint16_t)file_info.fsize / MAX_BYTES_TO_READ;
			uint16_t bytes_left_to_read = (uint16_t)file_info.fsize % MAX_BYTES_TO_READ;	// number of bytes left to read. 1 to 2047 bytes 
			uint16_t current_itreation = 0;
			
			// чтение файла по 2048 байт, по 4 сектора
			printf("--- File reading long data! Readed string: \n");
			while (current_itreation < reading_iterations){
				res = f_read(&file, readed_data, MAX_BYTES_TO_READ, &BytesReaded);
				if(res == FR_OK){
					usart1_send(readed_data, BytesReaded); // отправка в USART1 строк по 2048 байт сразу после чтения.
				}
				current_itreation++;
			}

			// вычитывание оставшихся байтов, которые не были считаны в итерациях по 2048 байт. Это кол-во байтов от 1 до 2047
			if (res == FR_OK){
				if(bytes_left_to_read > 0){
					res = f_read(&file, readed_data, bytes_left_to_read, &BytesReaded);
					if(res == FR_OK){
						usart1_send(readed_data, BytesReaded);	// отправка в USART1 оставшейся строки длинной от 1 до 2047 байт
					}
				}
			}
			
			if (res == FR_OK){
				printf("\n\n+++ Long file was readed successfully!\n");
				printf("+++ was readed %d bytes successfully!\n", (uint16_t)file_info.fsize);
			}
			else{
				printf("--- reading file FAILED! ErrorCode = %d \n", res);  
				f_close(&file);
				return res;
			}
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

		uint8_t *file_text =  malloc(128 * sizeof(uint8_t));		// reserve memory for string file_text
		sprintf(file_text, "Test string for file creation!!! \n");		// write test string into string file_text

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
		
    	// Получаем информацию о карте
		printf("--- SD-card Getting information! ---- \n");
    	SD_GetCardInfo(&SDCardInfo);
    	
		
    	// Выбор карты и настройка режима работы
		printf("--- SD-card Selection! ---- \n");
    	SD_SelectDeselect((uint32_t) (SDCardInfo.RCA << 16));
    	SD_SetDeviceMode(SD_POLLING_MODE);

		
		res = SD_CardMount();

		// чтение файла с карты памяти, если она инициализировалась верно.
		if (res == FR_OK){
			res = SD_CardFileRead();
		}
		else{
			printf("--- SD-card mounting failed... \n");
		}

		// создание нового файла на карте и запись в него тестовой строки
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

	