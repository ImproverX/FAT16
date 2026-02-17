;(перед компилированием преобразовать в KOI-8R)
CMSTR:	.EQU	00080h	; буфер командной строки
PAR1:	.EQU	0005Ch
PAR2:	.EQU	0006Ch
;
	.ORG	00100H
START:	LXI  D, ABOUT
	CALL	L_PRNT
	CALL    FS_RST	; сброс ФС
	LXI  D, ST_FAT
	JC	L_PRNT	; >> вывод ошибки и выход
;
	LXI  D, ALLFILS
	CALL    C_DIR	; команда DIR *
;
; ===== основной цикл =====
;
MAIN:	LXI  D, PROMPT	;***
	CALL	L_PRNT	;***
	LXI  H, CWD
	CALL	PRINT0	; вывод имени текущей директории
	MVI  A, '>'
	CALL	PRINTA
	LXI  D, CMSTR
	PUSH D
M1:	INR  E
M3:	PUSH D
	MVI  C, 001h
	CALL    00005h	; ввод символа с ожиданием
	POP  D
	CPI  00Dh	; <ВК>
	JZ	M2
	CPI  00Ah	; <ПС>
	JZ	M2
	CPI  00Ch	; <влево-вверх>
	JZ	M3
	CPI  01Fh	; <СТР>
	JZ	M3
	CPI  01Bh	; <АР2>
	JZ	M3
	CPI  009h	; <ТАБ>
	JZ	M3
	CPI  004h	; <F1>..<F4>
	JC	M3
;	JZ	MAIN	; <F5> -- не работает
	CPI  07Fh	; <ЗБ>
	JZ	M4
	STAX D
	JMP	M1	; цикл ввода команды
;	
M4:	CALL	L_BKSP	; надо затереть символ ЗБ
	MVI  A, 081h
	CMP  E
	JNC	M3	;>> это начало строки
	DCR  E
	CALL	L_BKSP	; надо затереть ещё символ
	JMP	M3
;	
L_BKSP:	MVI  A, 008h
	CALL	PRINTA
	MVI  A, ' '
	CALL	PRINTA
	MVI  A, 008h
	CALL	PRINTA
	RET	
;
M2:	XRA  A
	STAX D		; запись нуля в конец
	DCR  E
	MVI  A, 07Fh
	ANA  E
	POP  D
	STAX D		; запись длинны команды
	PUSH D
	LXI D,	NEWLINE
	CALL	L_PRNT
	POP  D		;LXI  D, CMSTR
	LDAX D
	CPI  003h
	JC	L_HELP	; <3 >> help
L_PARM:	LXI  H, T_CMD
	MVI  C, 0
L_PR0:	MOV  A, C
	CPI  10		; количество команд в списке
	JNC	L_HELP	; >>
	LXI  D, CMSTR+1
	MVI  B, 003h	; проверяемая длинна команды
L_LP0:	LDAX D
	CMP  M
	JNZ	L_NXTC	; следующая команда
	INX  D
	INX  H
	DCR  B
	JNZ	L_LP0
	LXI  H, MAIN	; адрес возврата
	PUSH H		; в стек
	LXI  H, T_CMDL
	MOV  A, C
	ADD  A
	MOV  E, A
	MVI  D, 000h
	DAD  D
	MOV  A, M
	INX  H
	MOV  H, M
	MOV  L, A
	LXI  D, CMSTR
	LDAX D		; длинна КС	
	PCHL
;
L_NXTC:	INX  H
	DCR  B
	JNZ	L_NXTC
L_NXT1:	INR  C
	JMP	L_PR0
;
; ===== конец основного цикла =====
;
T_CMD:	.DB "DIR", "CP ", "RUN", "DEL", "ERA"
	.DB "CD ", "MD ", "RD ", "REN", "EXI"
;
T_CMDL:	.DW P_DIR	; ссылки на ПП команд
	.DW P_COPY
	.DW P_RUN
	.DW P_DEL
	.DW P_DEL
	.DW P_CD
	.DW P_MD
	.DW P_RD
	.DW P_REN
	.DW P_EXIT	; выход
;
L_HELP:	LXI  D, SHELP
	CALL	L_PRNT
	JMP	MAIN
;
P_DIR:	CPI  004h
	LXI  D, ALLFILS
	JC	L_D1	; нет параметров
	LXI  D, CMSTR+5
L_D1:	CALL    C_DIR	; команда DIR *
	RET
;
P_CD:	CPI  004h
	JC	L_ERRF	; нет параметров
	LXI  D, CMSTR+4
	CALL    FS_CHDIR	; CD <DIR>
	JC	L_ERRF	; ошибка CD
	RET
;
P_RUN:	SUI  004h
	JC	L_ERRF	; нет параметров
	MOV  C, A
	LXI  D, CMSTR+5
	LXI  H, BUFF
	PUSH H		; сохраняем ссылку на ИФ
L_RN0:	LDAX D
	MOV  M, A
	CPI  ' '
	JZ	L_RN1	; до пробела
	INX  H
	INX  D
	DCR  C
	JNZ	L_RN0	; переносим имя запускаемого файла в BUFF
L_RN1:	MVI  M, 0	; добавляем ноль в конце имени
	LXI  H, CMSTR
	PUSH H
	MVI  C, 0FFh	; счётчик
L_RN2	INX  H
	LDAX D
	MOV  M, A
	INX  D
	INR  C
	ORA  A
	JNZ	L_RN2	; сдвиг параметра, до 0
	INX  H
	MVI  M, 0	; ещё один 0 в конец
	POP  H		; << CMSTR
	MOV  M, C	; длина параметров
	INX  H
L_RN3:	LXI  D, PAR1
	CALL	CPAR2	; копирование параметра 1
	LXI  D, PAR2
	CALL	CPAR2	; копирование параметра 2
	POP  H		; << BUFF
L_RN4:	MOV  A, M
	ORA  A
	JZ	L_RNEX	; >> нет расширения
	INX  H
	SUI  '.'
	JNZ	L_RN4	; ищем точку
	MOV  D, A	;
	MOV  E, A	; DE = 0
	MOV  A, M
	CPI  'R'
	JZ	L_RN5
	INR  D		; D = 1 (com)
	CPI  'C'
	JNZ	L_ERRF	; не ROM и не COM файл
L_RN5:	INX  H
	MOV  A, M
	CPI  '0'
	JZ	L_RN6
	INR  E		; E = 1 (rom/com)
	CPI  'O'
	JNZ	L_ERRF	; не ROM и не COM файл
L_RN6:	INX  H
	MOV  A, M
	CPI  'M'
	JNZ	L_ERRF	; не ROM и не COM файл
L_RN8:	MOV  A, E	
	STA	MVR2+2	; адрес запуска в MOVER = 0000h/0100h
	STA	LxR5+2	; адрес загрузки = 0000h/0100h
	PUSH D
	LXI  D, BUFF
	CALL	L_FLD	; загрузка файла в буфер
	POP  B
	JC	L_ERRF	; файл не найден
	INX  H		; HL = после загруженного файла
	SHLD	MVR0+1	; -- пересылка в 0100h
	SHLD	LxRST+1	; адрес ПП перемещения
	MOV  A, B
	ORA  A
	JNZ	L_RN7	; >> (com)
	OUT  010h	; отключение КД (для ROM)
	OUT  011h
	STA	LxR2	; затираем DAD SP
L_RN7:	DI
	POP  H		; очистка стека от адреса возврата
	LXI  H, 0000h
LxR2:	DAD SP		; меняется на NOP в случае ROM/R0M
	SHLD	MVR1+1	; ... <SP>
	PUSH D
	MVI  C, 18	; размер ПП перемещения
	LXI  D, MOVER	; откуда
	LHLD	LxRST+1	; куда
L_R1:	LDAX D
	MOV  M, A
	INX  D
	INX  H
	DCR  C
	JNZ	L_R1	; копирование ПП перемещения
LxR5:	LXI  H, 0100h	; куда перемещать
	POP  D		; = FLEN
	XCHG
	DAD  D
	XCHG
	INX  D
	XRA  A		; сброс признака С
	MOV  A, D
	RAR
	MOV  B, A
	MOV  A, E
	RAR
	MOV  C, A	; BC = (BC + Start)/2 -- сколько перемещать
	LXI SP, BufLD	; откуда
LxRST:	JMP	0000h	; поехали! (адрес меняется)
;
L_RNEX:	MVI  M, '.'
	INX  H
	PUSH H
	MVI  M, 'C'
	INX  H
	MVI  M, 'O'
	INX  H
	MVI  M, 'M'
	INX  H
	MVI  M, 0
	LXI  D, BUFF
	CALL	FS_FNDF	; поиск первого файла <ИФ>.COM
	POP  H
	LXI  D, 0101h	; (com)
	JNC	L_RN8	; >>
	PUSH H
	MVI  M, 'R'
	LXI  D, BUFF
	CALL	FS_FNDF	; поиск первого файла <ИФ>.ROM
	POP  H
	LXI  D, 0001h	; (rom)
	JNC	L_RN8	; >>
	INX  H
	MVI  M, '0'
	LXI  D, BUFF
	CALL	FS_FNDF	; поиск первого файла <ИФ>.ROM
	LXI  D, 0000h	; (r0m)
	JNC	L_RN8	; >>
	JMP	L_ERRF
;
MOVER:	POP  D
	MOV  M, E
	INX  H
	MOV  M, D
	INX  H
	DCX  B
	MOV  A, C
	ORA  B
MVR0:	JNZ	MOVER
MVR1:	LXI  SP, 0000h
	EI
MVR2:	JMP	0100h
;	
P_COPY:	CPI  004h
	JC	L_ERRF	; нет параметров
	MOV  E, A
	MVI  D, 0
	LXI  H, CMSTR+2
	DAD  D
	MOV  M, D	; добавляем ещё один ноль в конец строки параметров
	XRA  A
	STA	CPCNT+1	; счётчик файлов =0
	LXI  H, CMSTR+3
	LXI  D, PAR1
	CALL	CPAR	; разбор параметра 1
	LXI  D, PAR2
	CALL	CPAR	; разбор параметра 2
	LDA	PAR1	; применение параметров
	ANA  A
	JNZ	Z_UNC	; это копирование с диска --> в разработке
	LXI  D, CMSTR+4	; копирование из ФАТ
	CALL	F_OPEN	; открываем файл из DE 	<< загрузка файла в память //LXI  D, RFILE
	JC	CPY04	;L_ERRF	; файл не найден
CPY01:	LHLD	FLENH
	MOV  A, H
	ORA  L
	JNZ	CPY05	; большой файл (> 64кБ) -- пропускаем
	LDA	FLEN
	CPI  0C0h
	JNC	CPY05	; большой файл (>= 48кБ) -- пропускаем
	CALL	L_FLDN	; загрузка файла в буфер
	JC	CPY06	; ошибка загрузки
	PUSH D
	LXI  H, FNAME
	PUSH H
	CALL	PRN_NF	; печатать имя файла из adr(HL)
	POP  H
	LXI  D, BUFF+1	;PAR1+1
	MVI  C, 11
CPY02:	MOV  A, M
	STAX D
	INX  H
	INX  D
	DCR  C
	JNZ	CPY02	; переносим ИФ источника в БУФ
	POP  D	
	MOV  A, E	; FLEN/128
	RAL
	MOV  A, D
	RAL
	MOV  L, A
	MVI  A, 0
	RAL
	MOV  H, A
	MOV  A, E
	ANI  07Fh
	JZ	CPY03	; корректировка на неполный сектор
	INX  H
CPY03:	SHLD	WSECT+1	; количество секторов CP/M
	LXI  H,	PAR2
	LXI  D,	BUFF
	PUSH D
	MVI  B, 12	; счётчик
SAVE10:	MOV  A, M
	CPI  '?'
	JZ	SAVE11	; кроме вопросов
	STAX D
SAVE11:	INX  H
	INX  D
	DCR  B
	JNZ	SAVE10	; накладываем ИФ назначения на БУФ
	XCHG
	MVI  B, 36-12	; счётчик
SAVE12:	MVI  M, 0
	INX  H
	DCR  B
	JNZ	SAVE12	; очистка памяти далее
	LXI  D, ARROW
	CALL	L_PRNT	; стрелка
	POP  H
	PUSH H		;LXI  H, BUFF	;BUFF+1	;PAR1+1
	MOV  A, M
	INX  H
	ORA  A
	JZ	SAVE13
	ADI  040h
	CALL PRINTA
	MVI  A, ':'	
	CALL PRINTA
SAVE13:	CALL	PRN_NF	; печатать имени файла назначения из adr(HL)
	LXI  D, DOT3
	CALL	L_PRNT	; три точки
	POP  D
	PUSH D		; БУФ
	MVI  C, 19
	CALL	5	; удалить файл (если был)
	POP  D		; БУФ
	MVI  C, 22
	CALL	5	; создать файл
	LXI  H, BufLD	; DMA -- откуда брать данные
WSECT:	LXI  B, 00000h	; количество секторов CP/M
	XCHG
SAVE14:	PUSH B
	PUSH D		; BufLD
	MVI  C, 1AH	; установить DMA
	CALL	5
	LXI  D, BUFF	;PAR1
	MVI  C, 21
	CALL	5	; писать в файл
	POP  D
	POP  B
	ORA  A
	JNZ	SAVEER	; ошибка записи
	LXI  H, 128
	DAD  D
	XCHG		; DE = BufLD++
	DCX  B		; количество секторов -1
	MOV  A, B
	ORA  C
	JNZ	SAVE14
	LXI  D, BUFF	;PAR1
	MVI  C, 16
	CALL	5	; закрыть файл
	LXI  D, OK
	CALL	L_PRNT
	LXI  H,	CPCNT+1
	INR  M		; счётчик файлов +1
CPY05:	LXI  D, CMSTR+4	; маска файла
	CALL	F_OPNN	; поиск и открытие следующего файла
	JNC	CPY01	; повтор
CPY04:	LXI  D, CPDONE	; файл не найден >>> копирование завершено
	CALL	L_PRNT
CPCNT:	MVI  A, 0
	DAA
	CALL	PRHEX	; выводим количество скопированных файлов
	RET
;
CPY06:	LXI  D, ERROR
	CALL	L_PRNT	; ошибка загрузки
	JMP	CPY05

SAVEER:	CPI  2		; нет места
	JNZ	SAVE15
	LXI  D, NOROOM
	CALL	L_PRNT
SAVE15:	LXI  D, ERROR
	CALL	L_PRNT
	LXI D,	NEWLINE
	CALL	L_PRNT
	LXI  D, BUFF	;PAR1
	MVI  C, 16
	PUSH D
	CALL	5	; закрыть файл
	POP  D
	MVI  C, 19
	CALL	5	; удалить файл
	JMP	CPY04
;
P_DEL:
P_ERA:
P_MD:
P_RD:
P_REN:
Z_UNC:	LXI  D, UNDERC	; функция в разработке
	CALL	L_PRNT
	RET
;	
P_EXIT:	LXI  D, OK
	CALL	L_PRNT
	POP  H
	RET
;
L_ERRF:	LXI  D, ERROR
	CALL	L_PRNT
	XRA  A
	INR  A		; снятие признака Z
	RET
;
L_FLD:	CALL	F_OPEN	; открываем файл из DE 	<< загрузка файла в память //LXI  D, RFILE
	JC	L_FLNO	; файл не найден
	LHLD	FLENH
	MOV  A, H
	ORA  L
	JNZ	L_FLBG	; большой файл
L_FLDN:	LHLD	FLEN
	PUSH H
	PUSH H
	POP  B		; размер файла
	LXI  H, BufLD	; куда грузить
	PUSH H
	CALL	F_READ	; читаем файл в память
	POP  H		; = BufLD
	POP  D		; = FLEN
	DAD  D
	MVI  A, 01Ah
L_FL0:	MOV  M, A
	INR  L
	JNZ	L_FL0	; дополняем файл значениями 1А
	RET
;
L_FLNO:	LXI  D, NOFOUND
L_FLN1:	CALL	L_PRNT
	STC		; Выставляем признак С
	RET
;
L_FLBG:	LXI  D, BGFILE	; большой файл
	JMP	L_FLN1
;
CPAR:	PUSH D
	CALL	CPAR2	; разбор параметра
	POP  D
	INX  D
	LDAX D
	CPI  ' '
	RNZ		; первый символ не пробел
	MVI  C, 11
	MVI  A, '?'
CP26:	STAX D
	INX  D
	DCR  C
	JNZ	CP26	; заполняем вопросами
	RET
;
CPAR2:	MOV  A, M	; копирование параметра с разбором в вид СР/М (HL -- откуда, DE -- куда)
	ANA  A
	JZ	CPNP	; ошибка -- нет параметра
	CPI  ' '
	INX  H
	JZ	CPAR2	; пропускаем пробелы
	LXI  B, 0DF08h	; B = !(020h), С - счётчик
	MOV  A, M	; позиция [2] ":"
	DCX  H		; [1]
	CPI  ':'
	MVI  A, 0	; номер диска 0	
	JNZ	CP20	; в параметре нет буквы диска
	MOV  A, M
	SUI  041h
	JC	CPER	; буква диска < "A" -- ошибка
	CPI  008h
	JNC	CPER	; буква диска >= "I" -- ошибка
	INX  H		; [2] ":"
	INX  H		; [3] "F"
	INR  A
CP20:	STAX D		; сохраняем номер диска
	INX  D
	ANA  A
	JZ	CP21	; буквы диска не было
	MOV  A, M	; [1]/[3] "F"
	ANA  B		; == " " / 00
	JZ	CPN0	; только буква диска -- заполняем пробелами до конца
CP21:	MOV  A, M	; [2]/[4] "F"
	INX  H
	CPI  '*'
	JZ	CPST	; дополняем вопросами до расширения
	CPI  '.'
	JZ	CPS2	; дополняем пробелами до расширения
	STAX D		; сохраняем символ
	ANA  B		; == " " / 00
	JZ	CPSP	; дополняем пробелами до конца
	INX  D
	DCR  C
	JNZ	CP21	; < 8 символов, поиск точки
CP22:	MOV  A, M
	INX  H
	CPI  '.'
	JZ	CP23	; цикл до точки +1
	ANA  B
	JNZ	CP22	; цикл до точки/пробела/нуля
CPSP:	DCX  H
	MVI  A, 3	; дополнение пробелами до конца
	ADD  C
	MOV  C, A
	JMP	CPN2
;
CPS2:	CALL	CPN2	; дополнение пробелами
CP23:	MVI  C, 3	; счётчик 2
CP24:	MOV  A, M	; [х] расширение
	CPI  '*'
	JZ	CPS1	; дополняем вопросами до конца
	STAX D
	ANA  B		; == " " / 00
	JZ	CPN2	; дополняем пробелами до конца
	INX  D
	INX  H
	DCR  C
	JNZ	CP24	; < 3 символов
CP25:	MOV  A, M
	INX  H
	ANA  B		; == " " / 00
	JNZ	CP25	; цикл до конца параметра
	DCX  H
	RET		; >>> Z = 1
;	
CPVF:	MVI  A, '?'	; заполняем вопросами
	JMP	CPN1
;
CPNP:	XRA  A		; нет переметра -- диск 0
	STAX D
	INX  D
CPN0:	MVI  C, 11	; заполняем пробелами
CPN2:	MVI  A, ' '
CPN1:	STAX D
	INX  D
	DCR  C
	JNZ	CPN1
	RET		; >>> Z = 1
;	
CPST:	CALL	CPVF	; дополнение вопросами
	JMP	CP22	; к расширению
;
CPS1:	CALL	CPVF	; дополнение вопросами
	JMP	CP25	; на выход
;
CPER:	XRA  A		; не имя файла
	STAX D		; сохраняем диск 0
	INX  D
	MVI  C, 11
CPE0:	MOV  A, M
	STAX D
	ANA  B		; == " " / 00
	JZ	CPN2	;CPE3	;
	INX  D
	INX  H
	DCR  C
	JNZ	CPE0	; перенос строки в параметр, до пробела/нуля
CPE1:	MOV  A, M
	INX  H
	ANA  B
	JNZ	CPE1	; цикл до пробела/нуля
	DCX  H
	RET		; >>> Z = 0 (ошибка)
;
; ====================================================
;
; содержимое директории
; вход: DE -- ссылка на строку с маской файлов
;
C_DIR:	CALL   FS_FNDF	; поиск первого файла
	JC     C_DIRNO	; нет файлов ->>
C_DIR1:	PUSH D		; сохр. ссылку на маску файлов
	PUSH H
	;CALL   INKEY	; опрос нажатия кнопок 
	;CPI 0FFH	; приостановка вывода списка файлов если есть нажатие любой кнопки
	;JNZ $-5
	LXI  B, 11
	DAD  B
	MOV  A, M	; A = data(adr(HL+0Bh))
	POP  H
	ANI  8		; 0000 1000
	JNZ	C_DIR2	; -> признак длинного имени файла, пропускаем
	MVI  B, 8
	CALL	PRINTN	; печатать 8 символов из adr(HL)
	MVI  A, ' '
	CALL	PRINTA	; >>>>
	MVI  B, 3
	CALL	PRINTN	; печатать 3 символа из adr(HL)
	MVI  A, ' '
	CALL	PRINTA	; >>>>
	mov  A, M	; читаем следующий байт после имени файла (=20h)
	ani  010H
	jz	C_DIR11	; >> не директория
	LXI  D, SDIR	;_DIR
C_DIR5:	CALL	L_PRNT
	JMP	C_DIR12
;
C_DIR11:LXI  B, 00014h	;00012h
	DAD  B
	XRA  A
	CMP  M
	JNZ	C_DIR4	; файл больше 64к
	DCX  H
	CMP  M
	JNZ	C_DIR4	; файл больше 64к
	DCX  H
	mov A,M
	call   PRHEX	; >>>>
	dcx H
	mov A,M
	call   PRHEX	; >>>>
C_DIR12:LXI D,	SEPAR
	LDA	D_CNTC
	DCR  A
	JNZ	C_DIR3
	LXI  D,	NEWLINE
	MVI  A, 004h
C_DIR3:	STA	D_CNTC
	CALL	L_PRNT
C_DIR2:	POP  D		; маска, = ALLFILS
	CALL   FS_FNDN	; поиск следующего файла
	JNC    C_DIR1
	LDA	D_CNTC
	CPI  004h
	RZ
	LXI  D,	NEWLINE
	CALL	L_PRNT
	MVI  A, 004h
	STA	D_CNTC
	RET
;
C_DIR4:	LXI  D, MORE64K
	JMP	C_DIR5
;
C_DIRNO:LXI D,NOFILES
	JMP    L_PRNT
;
D_CNTC:	.DB 004h		; счётчик файлов в строке
;
;------------------------------------------------------------------
PRN_NF:	LXI  B, 00820h	;MVI  B, 8
			;MVI  C, ' '
PNF0:	MOV  A, M
	INX  H
	CMP  C
	CNZ	PRINTA
	DCR  B
	JNZ	PNF0	; Вывод 8 символов из adr(HL) без пробелов
	MVI  A, '.'
	CALL	PRINTA
	MVI  B, 3
PNF1:	MOV  A, M
	INX  H
	CALL	PRINTA
	DCR  B
	JNZ	PNF1	; Вывод 3 символов из adr(HL)
	RET
;
PRINTN:	MOV  A, M	; Вывод B символов из adr(HL)
	INX  H
	CALL	PRINTA
	DCR  B
	JNZ	PRINTN
	RET
;
PRINT0:	MOV A,M		; Вывод строки из adr(HL) до кода 00h
	ORA A
	RZ		; >> выход при 00h
	CALL PRINTA
	INX H
	JMP PRINT0
;
L_PRNT:	MVI  C, 009h	; Вывод последовательности символов (до "$")
	JMP     00005h
;
PRHEX:	MOV  B, A	; вывод значения A в HEX
	RRC
	RRC
	RRC
	RRC
	CALL    L_0245
	MOV  A, B
L_0245:	ORI	0F0h	; вывод полубайта в шестнадцатиричном формате
	DAA
	CPI	060h
	SBI	01Fh
PRINTA:	PUSH PSW	; вывод на экран
	PUSH B
	PUSH H
	PUSH D
	MOV  E, A
	MVI  C, 002h	; вывод на экран (1 символ)
	CALL    00005h
	POP  D
	POP  H
	POP  B
	POP  PSW
	RET
;
ST_FAT:	.DB " Ошибка FAT16!$",0
ERROR:	.DB " Ошибка$"
;
ALLFILS:.DB "*.*",0
;
ABOUT:	.DB " FAT16 для МДОС/РДС Вектор-06ц"
	.DB ", версия 0.2 (c)Improver,2026"
	.DB 0Dh, 0Ah, 0Ah, "$"
PROMPT:	.DB 0DH,0AH,"FAT:$"
SDIR:	.DB " DIR$"
MORE64K:.DB ">64K$"
OK:	.DB " OK"
NEWLINE:.DB 0DH,0AH,"$"
SEPAR:	.DB " | $"
ARROW:	.DB " -> $"
DOT3:	.DB "...$"
NOFILES:.DB "Нет файлов.$"
NOFOUND:.DB " Файл не найден.$"
BGFILE:	.DB " Файл больше 64Кб.$"
NOROOM:	.DB " Нет места на диске.$"
CPDONE:	.DB " Скопировано файлов: $"
SHELP:	.DB "Список доступных команд:", 0DH,0AH
	.DB "DIR [маска имени файла]"
	.DB " - показать содержимое"
	.DB " текущей директории", 0DH,0AH
	.DB "CD <директория>"
	.DB " - смена директории", 0DH,0AH
;;	.DB "MD <новое имя>"
;;	.DB " - создать директорию", 0DH,0AH
;;	.DB "RD <директория>"
;;	.DB " - удалить директорию", 0DH,0AH
	.DB "RUN <имя файла>.[COM|ROM|R0M]"
	.DB " - загрузить в память"
	.DB " и запустить выполнение", 0DH,0AH
;;	.DB "DEL(или ERA) <имя файла>"
;;	.DB " - удалить файл", 0DH,0AH
;;	.DB "REN <имя файла>"
;;	.DB " <новое имя файла>"
;;	.DB " - "
;;	.DB "переименовать файл", 0DH,0AH
	.DB "CP [Диск]:<ИФ откуда>"
	.DB " [Диск]:[ИФ куда] "
	.DB " - копировать файл", 0DH,0AH
	.DB "EXIT - выход в ОС", 0Dh,0Ah,"$"
UNDERC:	.DB "Функция в разработке", 0Dh,0Ah,"$"
;
BUF:	.EQU	(((($+689h) / 010h) + 1) * 010h)	; буфер для FAT16
#include "FAT16.inc"
#include "IDE.inc"
;
;BUF:	.EQU	((($ / 010h) + 1) * 010h)		; начало буферов для FAT16
BUFF:	.EQU	EBUF16					; БУФ -- блок управления файлом СР/М, 32-36 байт
BufLD:	.EQU	((((BUFF + 36) / 0100h) + 1) * 0100h)	; Буфер для загрузки данных, выравнивание по 0ХХ00h
;
EndProg:              ; метка конца программы - определение свободного места
;	.dw $-FS_RST
;	.dw BUF
;	.dw BufLD
	.END
