#include "gsm.h"

//local

char cmgr[] = "+CMGR";
char check_ok[] = "OK";

char clip[] = "+CLIP";
char cmti[] = "+CMTI";
char check_error[] = "ERROR";
char cops[] = "+COPS";
// tap lenh kiem tra thong tin thiet bi
char infor_cmd[] = "222";
///////////////////define_call
char all_call[] = "GOI";
char print_all[] = "IN1:";
char print_all_2[] = "IN2:";
char config_apn[] = "110";
char config_ip[] = "111";
char couter_send[] = "112";
char speedmax_cmd[] = "001";

char thongtin_cty[] = "005";
char thongtin_laixe[] = "0061";
char TTTX[] = "023";

char passWord[20];
char read_passWord[] = "123456";
char passWord_cmd[] = "1111";
char speed[speedLen];
char dat_version[] = "290414";
unsigned int timer_check_sms = 0;
unsigned int timer_read_sms = 0;
unsigned int speed_max;
unsigned char door_key;
char phone_1[phoneLen];

char ten_laixe_save_1[ten_laixeLen];
char number_phone1[phoneLen];
char so_gplx_save_1[so_gplxLen];
char ngaycap_gplx_save_1[ngaycapLen];
char handen_gplx_save_1[handenLen];
char data_flash[data_flash_len];
char data_flash_APN[50];
#define timeLen 4
char timeCheck[timeLen];

#define waiting     		1


#define speed_max_min   10
#define speed_max_max   120

//global
unsigned char queue_sms;
char data_sms[smsLen];
char Selection[Selec];
char sms_time[25];
char userCall_new[phoneLen];
char userCall[phoneLen];
char sms_reply[smsLen];
char phoneDrive[phoneLen];
char time_gps[20];
char time_gps_send[20];

char imei[imeiLen];

char rx_buffer0[RX_BUFFER_SIZE0];
unsigned char rx_wr_index0, rx_rd_index0, rx_counter0;
unsigned char rx_wr_index1;
unsigned char rx_rd_index1, rx_counter1;

char apn[apnLen];
char userName[userNameLen];
char passApn[passWordLen];
char ipServer[ipServerLen];
char tcpPort[tcpPortLen];

//Function
void gsm_on() {
	GPIOSetDir(GSM_POWER, OUTPUT);
	GPIOSetDir(GSM_RES, OUTPUT);

}
unsigned char init_gsm() {
	unsigned char i;
	flag_modem.modem = not_connect;
	GPIOSetValue(GSM_RES, LOW );
	delay_ms(1000);
	GPIOSetValue(GSM_RES, HIGH);
	delay_ms(5000);

	for (;;) {
		UART0_PrintString("AT+CPIN?\r");  // test xem co sim trong chip hay chua
		//led_status = ! led_status;
		delay_ms(1000);
		if (flag_modem.sim == 1) {
			UART0_PrintString("AT+CMGF=1\r");
			delay_ms(1000);
			// led_status=1;
			break;
		}

		if (++i >= 20) // check sim 30 lan ma ko nhan sim thi tu dong reset lai at64 de khoi dong lai con sim900
				{
			i = 0;
			// watchdog_on();

			UART0_PrintString("AT1\r");
			while (1)
				;
		}
	}

	UART0_PrintString("AT+CLIP=1\r");

	delay_ms(1000);
	i = 0;
	while (check_imei() != 0)
		;    // check imei o day
	//for(i=0 ;i<imeiLen; i++)imei_save[i]=imei[i];
	//UART0_PrintString("AT+CMGDA=\"DEL ALL\"\r");
	delay_ms(1000);

	flag_modem.modem = connected;

	UART0_PrintString("AT+COPS?\r");
	delay_ms(1000);
	//flag_modem.modem = not_connect;   // chua ket noi hoac chua biet da ket noi hay chua
	i = 0; // khi lat trang thai roi phai kiem tra lai 1 lan nua de xem co hoat dong hay chua
	delay_ms(2000);

	return 1;
}
void start_up_gsm() {
	unsigned char retry_sim900 = 0;
	unsigned int retry_update = 0;
	for (;;) {
		UART0_PrintString("AT\r");
		delay_ms(500);

		if (flag_modem.ok)
			break;
		else if (++retry_sim900 > 5) {
			retry_sim900 = 0;
			GPIOSetValue(GSM_POWER, HIGH);
			delay_ms(500);
			GPIOSetValue(GSM_POWER, LOW);
			GPIOSetValue(GSM_RES, HIGH);

			flag_modem.modem = not_connect;
			break;
		}
	}
}
void del_buffer(unsigned char number) {
	unsigned char i;
	if (number == 0) {
		rx_wr_index0 = 0;
		rx_counter0 = 0;
		for (i = 0; i < RX_BUFFER_SIZE0; i++)
			rx_buffer0[i] = 0;

	} else {
		rx_wr_index1 = 0;
		rx_counter1 = 0;
		for (i = 0; i < RX_BUFFER_SIZE1; i++)
			rx_buffer1[i] = 0;

	}
}

void upperCase() {
	unsigned char len, i;
	len = strlen(data_sms);
	for (i = 0; i < len; i++) {
		if (data_sms[i] >= 0x61 && data_sms[i] <= 0x7A)
			data_sms[i] = data_sms[i] - 0x20;   // chuyen sang dang chu hoa
	}
}
void data_SMS_read() {
	int i, k;
	for (i = 0; i < RX_BUFFER_SIZE0; i++) {
		if (rx_buffer0[i] == NULL )
			break;
		data_sms[i] = rx_buffer0[i];         // chuoi nay chua den 160 ky tu
	}
	data_sms[i] = NULL;
	memset(sms_reply, 160, NULL );
	//UART2_PrintString("da vao process_SMS \n\r ");
	sprintf(sms_reply, "D: %s;END %s;i=%d \r\n", data_sms, sms_time, i);
	UART2_PrintString(sms_reply);
	upperCase();   // chuyen sang chu hoa het
}
void process_SMS(void) {
	unsigned char i, k, j;

	for (i = 0;; i++) {
		if (rx_buffer0[i] == '"')
			break;    // rec unread
	}
	k = i + 1;

	for (i = k;; i++) {
		if (rx_buffer0[i] == '"')
			break;     // ,
	}
	k = i + 1;
	k = k + 2; ///k=k+5;
	for (i = k;; i++) {
		if (rx_buffer0[i] == '"')
			break;
		userCall[i - k] = rx_buffer0[i];        // 168...// +84168...
	}
	userCall[i - k] = NULL;

	for (j = 0; j < phoneLen; j++) {
		if (userCall[j] == NULL )
			break;
		userCall_new[j] = userCall[j];
	}
	userCall_new[j] = NULL;

	//memset(sms_reply,160,NULL);
	//sprintf(sms_reply,"1:%s,2_%s:,3_%s\r",rx_buffer0,userCall,userCall_new);
	//UART2_PrintString(sms_reply);
	k = i + 1;              // bo dau "

	k = k + 1;              // bo dau ,
	for (i = k;; i++) {
		if (rx_buffer0[i] == ',')
			break;
	}
	k = i + 2;
	for (i = k;; i++) {
		if (rx_buffer0[i] == '"')
			break;
		sms_time[i - k] = rx_buffer0[i];  // lay thoi gian cua tin nhan
	}

	memset(data_sms, smsLen, NULL );
	k = i + 1; // bo wa dau "      den phan noi dung tin nhan
	for (i = k; i < RX_BUFFER_SIZE0; i++) // tach du lieu cua noi dung tin  nhan tu vi tri cho den het toan bo tin nhan
			{
		if (rx_buffer0[i] == NULL )
			break;
		data_sms[i - k] = rx_buffer0[i];         // chuoi nay chua den 160 ky tu
	}
	data_sms[i - k] = NULL;

	upperCase();   // chuyen sang chu hoa het
//	memset(sms_reply, 160, NULL );
//	sprintf(sms_reply, "1:%s,2:%s:", data_sms, sms_time);
//	UART2_PrintString(sms_reply);

}
void process_gsm_data(void) {
	char chuoitam[20];
	unsigned char i, j;
	if (flag_system.imei_check) {
		flag_system.imei_check = 0;
		for (i = 0; i < imeiLen; i++) {
			if (rx_buffer0[i] == NULL )
				break;
			imei[i] = rx_buffer0[i];
		}
		imei[i] = NULL;
	}
	for (i = 0; i < 20; i++) {
		if (rx_buffer0[i]
				== ':'|| rx_buffer0[i] == ',' || rx_buffer0[i] == NULL)break;
		chuoitam[i] = rx_buffer0[i]; // cat lay 5 ky tu dau tien de xac dinh loai nao
	}
	chuoitam[i] = NULL;
	if (chuoitam[0] == '+' && chuoitam[1] == 'C' && chuoitam[2] == 'P'
			&& chuoitam[3] == 'I' && chuoitam[4] == 'N') {
		flag_modem.sim = 1;            // test sim     // sim ok ready
	} else if (strcmp(chuoitam, cmgr) == 0) {
		flag_modem.cmgr = 1;
		i = i + 3;
		for (j = i; j < 20; j++) {
			if (rx_buffer0[j] == '"')
				break;
			chuoitam[j - i] = rx_buffer0[j];
		}
		chuoitam[j - i] = NULL;

		process_SMS();
		//clearFlash();

//		memset(data_sms, smsLen, NULL );
//		//k = i + 1; // bo wa dau "      den phan noi dung tin nhan
//		for (i = 0; i < RX_BUFFER_SIZE0; i++) // tach du lieu cua noi dung tin  nhan tu vi tri cho den het toan bo tin nhan
//				{
//			if (rx_buffer0[i] == NULL )
//				break;
//			data_sms[i ] = rx_buffer0[i];         // chuoi nay chua den 160 ky tu
//		}
//		data_sms[i] = NULL;
//
//		upperCase();   // chuyen sang chu hoa het
//		memset(sms_reply, 160, NULL );
//		sprintf(sms_reply, "3:%s,4:%s:", data_sms, sms_time);
//		UART2_PrintString(sms_reply);

	} else if (chuoitam[0] == '+' && chuoitam[1] == 'C' && chuoitam[2] == 'U'
			&& chuoitam[3] == 'S' && chuoitam[4] == 'D') {
		i++;
		for (j = i; j < RX_BUFFER_SIZE0; j++) {
			if (rx_buffer0[j] == NULL || rx_buffer0[j] == '"')
				break;
		}
		i = j + 1;
		for (j = i; j < RX_BUFFER_SIZE0; j++) {
			if (rx_buffer0[j] == NULL || rx_buffer0[j] == '"')
				break;
			sms_reply[j - i] = rx_buffer0[j];
		}
		sms_reply[j - i] = NULL;

		flag_modem.cusd = 1;
	}
	//+CCLK
	else if (chuoitam[0] == '+' && chuoitam[1] == 'C' && chuoitam[2] == 'C'
			&& chuoitam[3] == 'L' && chuoitam[4] == 'K') {
		i = i + 3;  // bo qua : va khoang trang va dau "
		for (j = i; j < RX_BUFFER_SIZE0; j++) {
			if (rx_buffer0[j] == NULL || rx_buffer0[j] == '+')
				break;
			time_gps[j - i] = rx_buffer0[j];
			time_gps_send[j - i] = rx_buffer0[j];

		}
		time_gps[j - i] = NULL;
		time_gps_send[j - i] = NULL;

	}

	else if (strcmp(chuoitam, clip) == 0) {
		i = i + 3;
		for (j = i; j < RX_BUFFER_SIZE0; j++) {
			if (rx_buffer0[j] == NULL || rx_buffer0[j] == '"')
				break;
			userCall[j - i] = rx_buffer0[j];
		}
		userCall[j - i] = NULL;
		flag_modem.ring = ON;
	}
	//// APN///
	else if (strcmp(chuoitam, cops) == 0) {
		GPIOSetValue(BUZZER, HIGH);
		//delay_ms(50);
		//GPIOSetValue(BUZZER, LOW);
		i = i + 7;
		for (j = i; j < RX_BUFFER_SIZE0; j++) {
			if (rx_buffer0[j] == NULL || rx_buffer0[j] == '"')
				break;
			Selection[j - i] = rx_buffer0[j];
		}
		Selection[j - i] = NULL;
		// printf("Selection2: %s\r",Selection);

		flag_modem.op_selec = ON;
		GPIOSetValue(BUZZER, LOW);

	}
	/////END

	else if (chuoitam[0] == '+' && chuoitam[1] == 'C' && chuoitam[2] == 'O'
			&& chuoitam[3] == 'P' && chuoitam[4] == 'S') {
		GPIOSetValue(BUZZER, HIGH);

		//Delay(50);
		//GPIOSetValue(BUZZER, LOW); 
		i = i + 7;
		for (j = i; j < RX_BUFFER_SIZE0; j++) {
			if (rx_buffer0[j] == NULL || rx_buffer0[j] == '"')
				break;
			Selection[j - i] = rx_buffer0[j];
		}
		Selection[j - i] = NULL;
		// printf("Selection2: %s\r",Selection);

		flag_modem.op_selec = ON;
		//GPIOSetValue(BUZZER, HIGH);
		//Delay(50);
		GPIOSetValue(BUZZER, LOW);
	} else if (chuoitam[0] == '+' && chuoitam[1] == 'C' && chuoitam[2] == 'M'
			&& chuoitam[3] == 'G' && chuoitam[4] == 'S') {
		flag_modem.send_sms = complete;
		flag_system.send_data_flag = no_send;
	}

	else if (strcmp(chuoitam, check_ok) == 0) {
		flag_modem.ok = 1;
	} else if (strcmp(chuoitam, cmti) == 0) {
		flag_system.read_sms = 1;
		//EraseSectors();
	}

	else if (strcmp(chuoitam, check_error) == 0)
		flag_modem.error = 1;
	else if (chuoitam[0] == 'E' && chuoitam[1] == 'R' && chuoitam[2] == 'R'
			&& chuoitam[3] == 'O' && chuoitam[4] == 'R')
		flag_modem.error = 1;
	else if (chuoitam[0] == '+' && chuoitam[1] == 'C' && chuoitam[2] == 'M'
			&& chuoitam[3] == 'E' && chuoitam[4] == ' ' && chuoitam[5] == 'E'
			&& chuoitam[6] == 'R' && chuoitam[7] == 'R' && chuoitam[8] == 'O'
			&& chuoitam[9] == 'R')
		flag_modem.error = 1;
	else if (chuoitam[0] == '+' && chuoitam[1] == 'C' && chuoitam[2] == 'M'
			&& chuoitam[3] == 'S' && chuoitam[4] == ' ' && chuoitam[5] == 'E'
			&& chuoitam[6] == 'R' && chuoitam[7] == 'R' && chuoitam[8] == 'O'
			&& chuoitam[9] == 'R')
		flag_modem.error = 1;
	else if (chuoitam[0] == 'B' && chuoitam[1] == 'U' && chuoitam[2] == 'S'
			&& chuoitam[3] == 'Y')
		flag_modem.busy_call = 1;
	else if (chuoitam[0] == 'N' && chuoitam[1] == 'O' && chuoitam[2] == ' '
			&& chuoitam[3] == 'A' && chuoitam[4] == 'N' && chuoitam[5] == 'S'
			&& chuoitam[6] == 'W' && chuoitam[7] == 'E' && chuoitam[8] == 'R')
		flag_modem.no_answer = 1;
	else if (chuoitam[0] == 'N' && chuoitam[1] == 'O' && chuoitam[2] == ' '
			&& chuoitam[3] == 'D' && chuoitam[4] == 'I' && chuoitam[5] == 'A'
			&& chuoitam[6] == 'L' && chuoitam[7] == ' ' && chuoitam[8] == 'T'
			&& chuoitam[9] == 'O' && chuoitam[10] == 'N' && chuoitam[11] == 'E')
		flag_modem.no_dial_tone = 1;

	else if (chuoitam[0] == 'N' && chuoitam[1] == 'O' && chuoitam[2] == ' '
			&& chuoitam[3] == 'C' && chuoitam[4] == 'A' && chuoitam[5] == 'R'
			&& chuoitam[6] == 'R' && chuoitam[7] == 'I' && chuoitam[8] == 'E'
			&& chuoitam[9] == 'R')
		flag_modem.no_carrier = 1;
	else if (chuoitam[0] == 'C' && chuoitam[1] == 'O' && chuoitam[2] == 'N'
			&& chuoitam[3] == 'N' && chuoitam[4] == 'E' && chuoitam[5] == 'C'
			&& chuoitam[6] == 'T' && chuoitam[7] == ' ' && chuoitam[8] == 'O'
			&& chuoitam[9] == 'K')
		flag_gprs.connect_ok = 1;
	else if (chuoitam[0] == 'C' && chuoitam[1] == 'O' && chuoitam[2] == 'N'
			&& chuoitam[3] == 'N' && chuoitam[4] == 'E' && chuoitam[5] == 'C'
			&& chuoitam[6] == 'T' && chuoitam[7] == ' ' && chuoitam[8] == 'F'
			&& chuoitam[9] == 'A' && chuoitam[8] == 'I' && chuoitam[9] == 'L')
		flag_gprs.connect_ok = 0;
	else if (strcmp(chuoitam, "SEND OK")) {
		flag_gprs.send_gprs_ok = 1;
	} else if (chuoitam[0] == 'C' && chuoitam[1] == 'L' && chuoitam[2] == 'O'
			&& chuoitam[3] == 'S' && chuoitam[4] == 'E')
		flag_gprs.connect_ok = 0;     // dong ket noi server
	else if (strcmp(chuoitam, "CLOSE OK"))
		flag_gprs.connect_ok = 0;

	else if (chuoitam[0] == 'A' && chuoitam[1] == 'L' && chuoitam[2] == 'R'
			&& chuoitam[3] == 'E' && chuoitam[4] == 'A' && chuoitam[5] == 'D'
			&& chuoitam[6] == 'Y' && chuoitam[7] == ' ' && chuoitam[8] == 'C')
		flag_gprs.connect_ok = 1;
	else if (chuoitam[0] == 'T' && chuoitam[1] == 'C' && chuoitam[2] == 'P'
			&& chuoitam[3] == ' ' && chuoitam[4] == 'E' && chuoitam[5] == 'R'
			&& chuoitam[6] == 'R' && chuoitam[7] == 'O' && chuoitam[8] == 'R')
		flag_gprs.deactive_pdp = 1;
	else if (chuoitam[0] == '+' && chuoitam[1] == 'P' && chuoitam[2] == 'D'
			&& chuoitam[3] == 'P' && chuoitam[4] == ':' && chuoitam[5] == ' '
			&& chuoitam[6] == 'D' && chuoitam[7] == 'E' && chuoitam[8] == 'A'
			&& chuoitam[9] == 'C' && chuoitam[10] == 'T')
		flag_gprs.deactive_pdp = 1;
	else if (chuoitam[0] == 'S' && chuoitam[1] == 'T' && chuoitam[2] == 'A'
			&& chuoitam[3] == 'T' && chuoitam[4] == 'E')

			{
		i = i + 2;  // bo qua : va khoang trang
		for (j = i; j < 20; j++) {
			if (rx_buffer0[j] == NULL )
				break;
			chuoitam[j - i] = rx_buffer0[j];
		}
		chuoitam[j - i] = NULL;
		if (chuoitam[0] == 'P' && chuoitam[1] == 'D' && chuoitam[2] == 'P'
				&& chuoitam[3] == ' ' && chuoitam[4] == 'D'
				&& chuoitam[5] == 'E' && chuoitam[6] == 'A'
				&& chuoitam[7] == 'C' && chuoitam[8] == 'T')
			flag_gprs.deactive_pdp = 1;
		else if (chuoitam[0] == 'T' && chuoitam[1] == 'C' && chuoitam[2] == 'P'
				&& chuoitam[3] == ' ' && chuoitam[4] == 'C'
				&& chuoitam[5] == 'L' && chuoitam[6] == 'O'
				&& chuoitam[7] == 'S' && chuoitam[8] == 'E')
			flag_gprs.deactive_pdp = 1;
	}

}

unsigned char check_imei() {
	unsigned char i = 0;
	// for(;;)
	// {
	UART0_PrintString("AT+GSN\r");    // kiem tra imei
	flag_system.imei_check = 1;
	delay_ms(1000);
	flag_system.imei_check = 0;
	if (strlen(imei) > 10)
		return 0; // neu imei da co thi tra ve 0
	if (++i >= 10)
		return 1;
	/// }
}
void send_sms_func(char *smsString) {
	char buffer[200];

	memset(buffer, 200, NULL );
	delay_ms(500);
	sprintf(buffer, "AT+CMGS=\"%s\"\r", userCall_new);
	//sprintf(buffer,"AT+CMGS=\"0914777116\"\r");
	UART0_PrintString(buffer);
	delay_ms(1000);
	memset(buffer, 200, NULL );
	delay_ms(300);
	sprintf(buffer, "%s%c", smsString, ctrl_z);
	delay_ms(300);
	UART0_PrintString(buffer);
	flag_modem.send_sms = waiting;
	flag_system.timeout_sms = 0;
	flag_modem.ok = 0;
	flag_modem.error = 0;
	timer_check_sms = 0;
	while (flag_modem.send_sms == waiting) {
		if (flag_system.timeout_sms || flag_modem.error) {
			flag_modem.send_sms = complete;
			flag_system.send_data_flag = no_send;
			break;
		}
	}
	memset(userCall, phoneLen, NULL );
}

void process_command() {

	unsigned char i, k;
	char chuoitam[20];
	char co_loi = 0;
	char a[3];
	char ngay[3];
	char thang[3];
	char nam[3];
	char gio[3];
	char phut[3];
	char giay[3];
	char ngay_thang_nam[10];
	char gio_phut_giay[10];
	char temp_from_time[10];
	char temp_printDate[10];

//#if _DEBUG==1
//	UART2_PrintString("vao  process_command\r\n");
//#endif
	for (i = 0; i < smsLen; i++) {
		if (data_sms[i] == NULL || data_sms[i] == ' ')
			break;
		chuoitam[i] = data_sms[i];
	}
	chuoitam[i] = NULL;
	//memset(sms_reply, smsLen, NULL );
	//sprintf(sms_reply, "chuoitam: %s,data_sms: %s", chuoitam, data_sms);
	//UART2_PrintString(sms_reply);
	//delay_ms(500);
	if (strcmp(chuoitam, read_passWord) == 0) {
		READ_APN_IP_SPEED();
		read_basic_infor();
		sprintf(passWord, "%s", flash_data_APN_IP.pass_save);
		if (passWord[0] == NULL || atoi(passWord) <= 1 || passWord[0] == "")
			sprintf(passWord, "999999");
		sprintf(sms_reply, "ID:%s,IP:%s,%s,PASS:%s,%s,SD: %d", imei,
				flash_data_APN_IP.ipServer_save, flash_data_APN_IP.tcpPort_save,
				flash_data_APN_IP.pass_save, passWord, flag_system.card_status);
		flag_system.send_data_flag = smsMode;
		flag_gprs.bit_data_flash_apn = 1;
	}
	if (strcmp(chuoitam, passWord) == 0) {
		k = i + 1;
		for (i = k; i < smsLen; i++) {
			if (data_sms[i] == NULL || data_sms[i] == ' ')
				break;

			chuoitam[i - k] = data_sms[i];
		}
		chuoitam[i - k] = NULL;
		memset(sms_reply, smsLen, NULL );

		if (strcmp(chuoitam, passWord_cmd) == 0) {
			k = i + 1;
			for (i = k; i < smsLen; i++) {
				if (data_sms[i] == NULL )	//|| data_sms[i] == ' ')
					break;

				passWord[i - k] = data_sms[i];
			}
			passWord[i - k] = NULL;

			sprintf(sms_reply, "Password change: %s", passWord);
			READ_APN_IP_SPEED();
			sprintf(data_flash_APN, "%s,%s,%s,%s,%s,%s,%s,\n\r",
					flash_data_APN_IP.apn_save, flash_data_APN_IP.userName_save,
					flash_data_APN_IP.passWord_save,
					flash_data_APN_IP.ipServer_save,
					flash_data_APN_IP.tcpPort_save,
					flash_data_APN_IP.speed_save, passWord);
			flag_gprs.bit_data_flash_apn = 1;
		}

		else if (strcmp(chuoitam, infor_cmd) == 0) {
			sprintf(sms_reply,
					"LPC:%s,TCP:%s,%s,%s,IP:%s,%s,C:%d,P:%s,S:%s,ID:%s,LAT:%s,LOG:%s,D:%d,K:%d,SL:%d",
					dat_version, apn, userName, passApn, ipServer, tcpPort,
					counter_send_gps, flash_data.phone,
					flash_data_APN_IP.speed_save, imei, latitude, longitude,
					flag_system.status_door, flag_system.status_key,
					flag_system.sleep);	//speed
		}

		/// setup van toc
		else if (strcmp(chuoitam, speedmax_cmd) == 0)    // set toc do toi da
				{
			k = i + 1;
			for (i = k; i < smsLen; i++) {
				if (data_sms[i] == NULL || data_sms[i] == ' ')
					break;
				a[i - k] = data_sms[i];
			}
			a[i - k] = NULL;
			speed_max = atoi(a);
			if (speed_max < speed_max_min || speed_max > speed_max_max) {
				sprintf(sms_reply,
						"Toc do cai dat %d vuot nguong cho phep %d < V < %d",
						speed_max, speed_max_min, speed_max_max);
				//speed_max = speed_max_save;
			} else {
				//speed_max_save = speed_max;
				sprintf(sms_reply, "Set speed_max :%d ok", speed_max);
			}
			READ_APN_IP_SPEED();
			sprintf(data_flash_APN, "%s,%s,%s,%s,%s,%s,%s,\n\r",
					flash_data_APN_IP.apn_save, flash_data_APN_IP.userName_save,
					flash_data_APN_IP.passWord_save,
					flash_data_APN_IP.ipServer_save,
					flash_data_APN_IP.tcpPort_save, a,
					flash_data_APN_IP.pass_save);
			flag_gprs.bit_data_flash_apn = 1;
		}

		///////end

		//// setup APN

		else if (strcmp(chuoitam, config_apn) == 0)    // set toc do toi da
				{
			k = i + 1;
			for (i = k; i < smsLen; i++) {
				if (data_sms[i] == NULL || data_sms[i] == ',')
					break;

				apn[i - k] = data_sms[i];
			}

			apn[i - k] = NULL;

			k = i + 1;
			for (i = k; i < smsLen; i++) {
				if (data_sms[i] == NULL || data_sms[i] == ',')
					break;

				userName[i - k] = data_sms[i];
			}

			userName[i - k] = NULL;

			k = i + 1;
			for (i = k; i < smsLen; i++) {
				if (data_sms[i] == NULL || data_sms[i] == ',')
					break;
				passApn[i - k] = data_sms[i];
			}

			passApn[i - k] = NULL;

			sprintf(sms_reply,
					"APN:%s,userName:%s,passApn:%s.XIN QUY KHACH KIEM TRA !",
					apn, userName, passApn);
			if (userName[0] == NULL )
				sprintf(userName, "NULL");
			if (passApn[0] == NULL )
				sprintf(passApn, "NULL");

			READ_APN_IP_SPEED();
			sprintf(data_flash_APN, "%s,%s,%s,%s,%s,%s,%s,\n\r", apn, userName,
					passApn, flash_data_APN_IP.ipServer_save,
					flash_data_APN_IP.tcpPort_save,
					flash_data_APN_IP.speed_save, flash_data_APN_IP.pass_save);
			flag_gprs.bit_data_flash_apn = 1;
		}

		//////END//////
/// setup IP_PORT/// config_ip
		else if (strcmp(chuoitam, config_ip) == 0) {
			k = i + 1;
			for (i = k; i < smsLen; i++) {
				if (data_sms[i] == NULL || data_sms[i] == ',')
					break;
				if ((data_sms[i] != '.'
						&& (data_sms[i] > 0x39 || data_sms[i] < 0x30))
						|| (i - k) >= ipServerLen) {
					co_loi = 1;
					goto xu_ly_loi;
				}
				ipServer[i - k] = data_sms[i];
			}

			ipServer[i - k] = NULL;
			k = i + 1;
			for (i = k; i < smsLen; i++) {
				if (data_sms[i] == NULL || data_sms[i] == ',')
					break;
				if (data_sms[i]
						> 0x39|| data_sms[i] < 0x30 || (i-k)>=tcpPortLen) {co_loi=2;
				goto xu_ly_loi;
			}
				tcpPort[i - k] = data_sms[i];

			}
			tcpPort[i - k] = NULL;
			sprintf(sms_reply, "IP:%s,PORT:%s.XIN QUY KHACH KIEM TRA !",
					ipServer, tcpPort);
			READ_APN_IP_SPEED();
			sprintf(data_flash_APN, "%s,%s,%s,%s,%s,%s,%s,\n\r",
					flash_data_APN_IP.apn_save, flash_data_APN_IP.userName_save,
					flash_data_APN_IP.passWord_save, ipServer, tcpPort,
					flash_data_APN_IP.speed_save, flash_data_APN_IP.pass_save);
			flag_gprs.bit_data_flash_apn = 1;
		}
		///END////////////////
		//// lenh set counter
		else if (strcmp(chuoitam, couter_send) == 0) {
			k = i + 1;

			for (i = k; i < smsLen; i++) {
				if (data_sms[i] == NULL || data_sms[i] == ' ')
					break;
				if (data_sms[i] > 0x39 || data_sms[i] < 0x30
						|| (i - k) >= timeLen) {
					co_loi = 3;
					goto xu_ly_loi;
				}
				timeCheck[i - k] = data_sms[i];
			}

			timeCheck[i - k] = NULL;
			counter_send_gps = atoi(timeCheck);
			sprintf(sms_reply, "THOI GIAN GUI:%d.XIN QUY KHACH KIEM TRA !",
					counter_send_gps );

		}

		//end couter

		else {
			sprintf(sms_reply, "xin kiem tra lai ! %s", chuoitam);
		}

		flag_system.send_data_flag = smsMode;
	} else if (strcmp(chuoitam, all_call) == 0) {
		k = i + 1;
		for (i = k; i < smsLen; i++) {
			if (data_sms[i] == NULL || data_sms[i] == ' ' || data_sms[i] == ',')
				break;
			if ((data_sms[i] != '.'
					&& (data_sms[i] > 0x39 || data_sms[i] < 0x30))
					|| (i - k) >= ipServerLen) {
				co_loi = 1;
				goto xu_ly_loi;
			}
			phone_1[i - k] = data_sms[i];
		}
		phone_1[i - k] = NULL;
		xu_ly_loi: if (co_loi != 0)

		{
			//for (i=0;i<phoneLen;i++) number_phone_1[i] = phoneNumber_N32_save[i];
			//for (i=0;i<apnLen;i++) setup_call_sms[i]=setup_call_sms_save[i];

			sprintf(sms_reply, "Gia tri cai dat khong hop le %d", co_loi);
		} else {
			//for (i=0;i<phoneLen;i++) phoneNumber_N32_save[i] = number_phone_1[i];
			//for (i=0;i<apnLen;i++) setup_call_sms_save[i]=setup_call_sms[i];

			sprintf(sms_reply, "DT:%s", phone_1);
			flag_modem.upinfo = 1;
		}
		flag_system.send_data_flag = smsMode;
	}

/// thong tin cong ty

	else if (strcmp(chuoitam, thongtin_cty) == 0) {
		k = i + 1;
		for (i = k; i < smsLen; i++) {
			if (data_sms[i] == NULL || data_sms[i] == ',')
				break;
			tencongty[i - k] = data_sms[i];

		}
		tencongty[i - k] = NULL;

		k = i + 1;
		for (i = k; i < smsLen; i++) {
			if (data_sms[i] == NULL || data_sms[i] == ',')
				break;
			diachi[i - k] = data_sms[i];

		}
		diachi[i - k] = NULL;

		k = i + 1;
		for (i = k; i < smsLen; i++) {
			if (data_sms[i] == NULL || data_sms[i] == ',')
				break;
			so_vin[i - k] = data_sms[i];

		}
		so_vin[i - k] = NULL;

		k = i + 1;
		for (i = k; i < smsLen; i++) {
			if (data_sms[i] == NULL || data_sms[i] == ',')
				break;
			bien_soxe[i - k] = data_sms[i];

		}
		bien_soxe[i - k] = NULL;

		sprintf(sms_reply, "%s,%s,%s,%s", tencongty, diachi, so_vin, bien_soxe);

		flag_system.send_data_flag = smsMode;
		//LPT
		read_basic_infor();
		sprintf(data_flash, "%s,%s,%s,%s,%s,%s,%s,%s,%s,\n\r", tencongty,
				diachi, so_vin, bien_soxe, flash_data.ownerName,
				flash_data.license, flash_data.license_iss_date,
				flash_data.license_exp_date, flash_data.phone);
		flag_gprs.bit_data_infor = 1;

	}
/////////end/

	else if (strcmp(chuoitam, thongtin_laixe) == 0) {
		// 0061 Nguyen Van A,0904..,A557799,15/09/2013,15/09/2020    //TX1
		k = i + 1;
		for (i = k; i < smsLen; i++) {
			if (data_sms[i] == NULL || data_sms[i] == ',')
				break;
			ten_laixe_save_1[i - k] = data_sms[i];

		}
		ten_laixe_save_1[i - k] = NULL;

		k = i + 1;
		//////// phone
		for (i = k; i < smsLen; i++) {
			if (data_sms[i] == NULL || data_sms[i] == ',' || data_sms[i] == 'A')
				break;
			if ((data_sms[i] != '.'
					&& (data_sms[i] > 0x39 || data_sms[i] < 0x30))
					|| (i - k) >= ipServerLen)
				break;
			// {
			//  co_loi=1;
			//  goto xu_ly_loi;
			// }
			number_phone1[i - k] = data_sms[i];

		}
		number_phone1[i - k] = NULL;

		////////////////
		k = i + 1;
		for (i = k; i < smsLen; i++) {
			if (data_sms[i] == NULL || data_sms[i] == ',')
				break;

			so_gplx_save_1[i - k] = data_sms[i];
		}
		so_gplx_save_1[i - k] = NULL;

		k = i + 1;
		for (i = k; i < smsLen; i++) {
			if (data_sms[i] == NULL || data_sms[i] == ',')
				break;

			ngaycap_gplx_save_1[i - k] = data_sms[i];
		}

		ngaycap_gplx_save_1[i - k] = NULL;

		k = i + 1;
		for (i = k; i < smsLen; i++) {
			if (data_sms[i] == NULL || data_sms[i] == ',')
				break;
			// handen_gplx[i-k] = data_sms[i];
			handen_gplx_save_1[i - k] = data_sms[i];
		}

		handen_gplx_save_1[i - k] = NULL;

		if (ten_laixe_save_1[0] == NULL )
			sprintf(sms_reply, "No. Ten lai xe chua co");
		else
			sprintf(sms_reply, "Ye. TX1:%s, PHONE:%s", ten_laixe_save_1,
					number_phone1);
		flag_system.send_data_flag = smsMode;
		//LPT
		read_basic_infor();
		sprintf(data_flash, "%s,%s,%s,%s,%s,%s,%s,%s,%s,\n\r",
				flash_data.company, flash_data.address, flash_data.vin_No,
				flash_data.id_device, ten_laixe_save_1, so_gplx_save_1,
				ngaycap_gplx_save_1, handen_gplx_save_1, number_phone1);
		flag_gprs.bit_data_infor = 1;

	}

/////////end
	/// thong tin tai xe
	else if (strcmp(chuoitam, TTTX) == 0) {
		sprintf(sms_reply, "TX:%s:%s", flash_data.ownerName, flash_data.phone);
		flag_system.send_data_flag = smsMode;
	}
	////end////

///////// printer
	else if (strcmp(chuoitam, print_all) == 0)		//in du lieu trong ngay
			{
		// IN1: 05 08 12 08 30 15
		k = i + 1;
		for (i = k; i < smsLen; i++) {
			if (data_sms[i] == ' ' || data_sms[i] == NULL )
				break;
			ngay[i - k] = data_sms[i];
		}
		ngay[i - k] = NULL;
		k = i + 1;
		for (i = k; i < smsLen; i++) {
			if (data_sms[i] == ' ' || data_sms[i] == NULL )
				break;
			thang[i - k] = data_sms[i];
		}
		thang[i - k] = NULL;
		k = i + 1;
		for (i = k; i < smsLen; i++) {
			if (data_sms[i] == ' ' || data_sms[i] == NULL )
				break;
			nam[i - k] = data_sms[i];
		}
		nam[i - k] = NULL;
		k = i + 1;
		for (i = k; i < smsLen; i++) {
			if (data_sms[i] == ' ' || data_sms[i] == NULL )
				break;
			gio[i - k] = data_sms[i];
		}
		gio[i - k] = NULL;
		k = i + 1;
		for (i = k; i < smsLen; i++) {
			if (data_sms[i] == ' ' || data_sms[i] == NULL )
				break;
			phut[i - k] = data_sms[i];
		}
		phut[i - k] = NULL;
		k = i + 1;
		for (i = k; i < smsLen; i++) {
			if (data_sms[i] == ' ' || data_sms[i] == NULL )
				break;
			giay[i - k] = data_sms[i];
		}
		giay[i - k] = NULL;
		sprintf(chuoitam, "%s/%s/%s,%s:%s:%s", nam, thang, ngay, gio, phut,
				giay);
		sprintf(gio_phut_giay, "%s:%s:%s", gio, phut, giay);
		sprintf(ngay_thang_nam, "%s/%s/%s", ngay, thang, nam);
		sprintf(temp_from_time, "%s0000", gio);
		sprintf(temp_printDate, "%s%s%s", ngay, thang, nam);
		if (flag_system.card_status) {

			//Get information from sdcard and put them to global data
			if (read_basic_infor() == 1) {
				//KHN_Print("07/04/14", "16:18:00", "1234567890", "GSB4324235665");

				KHN_Print(ngay_thang_nam, gio_phut_giay, "CHUA CN", "CHUA CN",
						time_gps_card, temp_from_time, temp_printDate);
			} else {
				UART2_PrintString("Co loi xay ra khi in\r\n");
				UART2_PrintString("Ma loi :  bsi-01\r\n");
			}

			//ket_qua();
			sprintf(sms_reply, "Da in xong %s", chuoitam);
			flag_system.led_check = 0;
		} else
			sprintf(sms_reply, "Chua lap the nho");

		flag_system.send_data_flag = smsMode;
	}

	//// in2:
	else if (strcmp(chuoitam, print_all_2) == 0)      //in du lieu trong ngay
			{      // 0000 004 11/10/25,08:30:00 20:00:00
				   // IN: 05 08 12 08 30 15    // in thoi gian 05/08/12 08:30:15

		k = i + 1;
		for (i = k; i < smsLen; i++) {
			if (data_sms[i] == ' ' || data_sms[i] == NULL )
				break;
			ngay[i - k] = data_sms[i];
		}
		ngay[i - k] = NULL;
		k = i + 1;
		for (i = k; i < smsLen; i++) {
			if (data_sms[i] == ' ' || data_sms[i] == NULL )
				break;
			thang[i - k] = data_sms[i];
		}
		thang[i - k] = NULL;
		k = i + 1;
		for (i = k; i < smsLen; i++) {
			if (data_sms[i] == ' ' || data_sms[i] == NULL )
				break;
			nam[i - k] = data_sms[i];
		}
		nam[i - k] = NULL;
		k = i + 1;
		for (i = k; i < smsLen; i++) {
			if (data_sms[i] == ' ' || data_sms[i] == NULL )
				break;
			gio[i - k] = data_sms[i];
		}
		gio[i - k] = NULL;
		k = i + 1;
		for (i = k; i < smsLen; i++) {
			if (data_sms[i] == ' ' || data_sms[i] == NULL )
				break;
			phut[i - k] = data_sms[i];
		}
		phut[i - k] = NULL;
		k = i + 1;
		for (i = k; i < smsLen; i++) {
			if (data_sms[i] == ' ' || data_sms[i] == NULL )
				break;
			giay[i - k] = data_sms[i];
		}
		giay[i - k] = NULL;

		sprintf(chuoitam, "%s/%s/%s,%s:%s:%s", nam, thang, ngay, gio, phut,
				giay);
		sprintf(gio_phut_giay, "%s:%s:%s", gio, phut, giay);
		sprintf(ngay_thang_nam, "%s/%s/%s", ngay, thang, nam);
		sprintf(temp_from_time, "%s0000", gio);
		sprintf(temp_printDate, "%s%s%s", ngay, thang, nam);

		sprintf(chuoitam, "%s/%s/%s,%s:%s:%s", nam, thang, ngay, gio, phut,
				giay);

		if (flag_system.card_status) {

			if (read_basic_infor() == 1) {
				//KHN_Print("07/04/14", "16:18:00", "1234567890", "GSB4324235665");

				KHN_Print_Infor(ngay_thang_nam, gio_phut_giay, time_gps_card,
						imei);
			} else {
				UART2_PrintString("Co loi xay ra khi in\r\n");
				UART2_PrintString("Ma loi :  bsi-01\r\n");
			}
			//find_file_2(chuoitam);
			sprintf(sms_reply, "Da in xong %s", chuoitam);
		} else
			sprintf(sms_reply, "Chua lap the nho");
		flag_system.send_data_flag = smsMode;
	}

	///end

///////////END

}
void ring_func() {
//led_status=0;
	flag_modem.ring = 0;
	UART0_PrintString("ATH\r");
	delay_ms(1000);

}
