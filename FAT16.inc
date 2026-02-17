;BUF:	.EQU	(((EndProg / 010h) + 1) * 010h)	; буфер чтения/записи
BUFDIR:	.EQU	BUF+0200h	; буфер для директории
CWD:	.EQU	BUF+0400H	; текущий путь, для отображения
;
FATSEC:	.EQU	BUF+0500H	; номер первого сектора первой таблицы FAT
ROOTSEC:.EQU	FATSEC+4	; номер первого сектора корневой директории
DATASEC:.EQU	ROOTSEC+4	; первый сектор области данных
ROOTLEN:.EQU	DATASEC+4	; количество дескрипторов файлов в корневом каталоге
ROOTLNS:.EQU	ROOTLEN+2	; размер корневого каталога (в секторах)
SECINBL:.EQU	ROOTLNS+2	; число секторов в кластере
FAT_NXT:.EQU	SECINBL+1	; -- для FAT12/32, отключено
BUFSEC:	.EQU	FAT_NXT+3	; номер сектора, который сейчас считан в буфер
BLKLEN:	.EQU	BUFSEC+4	; осталось прочитать/записать секторов в кластере
BLKSEC:	.EQU	BLKLEN+1	; номер сектора для чтения/записи
BNEXT:	.EQU	BLKSEC+4	; номер следующего кластера файла (2 байта при FAT16)
FBPOS:	.EQU	BNEXT+4
FBLEN:	.EQU	FBPOS+2
DIRREC:	.EQU	FBLEN+2		; номер записи в директории (0..Fh)
DIRSEC:	.EQU	DIRREC+1	; номер сектора директории (4 байта)
CWDBLK:	.EQU	DIRSEC+4	; номер кластера текущей директории
FLEN:	.EQU	CWDBLK+4	; длина файла (сохраняется при открытии)
FLENH:	.EQU	FLEN+2		; старшее слово длины файла (сохраняется при открытии)
FNAME:	.EQU	FLENH+2		; имя файла (сохраняется при открытии)
FNAMES:	.EQU	FNAME+11	; имя файла в виде строки "8.3" (сохраняется при открытии)
CORLEN:	.EQU	FNAMES+13	; признак необходимости коррекции длины файла
BUFCHG:	.EQU	CORLEN+1	; признак "буфер был изменён"
EBUF16:	.EQU	BUFCHG+2	; конец буферов и данных FAT16
;
;+FS_RST	; перечитать ФС
;+FS_FNFF	; поиск первого файла, 			DE - имя файла/маска (строка "8.3"), выход: HL - ссылка на строку записи FAT
;+FS_FNFN	; поиск следующего файла, 		DE - имя файла/маска (строка "8.3"), выход: HL - ссылка на строку записи FAT
;+FS_FNDF	; поиск первого файла/директории, 	DE - имя файла/маска (строка "8.3"), выход: HL - ссылка на строку записи FAT
;+FS_FNDN	; поиск следующего файла/директоиии, 	DE - имя файла/маска (строка "8.3"), выход: HL - ссылка на строку записи FAT
;+F_OPEN	; открытие файла, 			DE - имя файла/маска (строка "8.3"), выход: C==1 если файл не найден, +++
;+F_OPNN	; открытие следующего файла
;+F_READ	; HL - addr for store readed data, BC - size of readed data
;+F_WRITE	; HL - адрес начала данных, BC - размер блока данных для записи в файл
;+FS_CHDIR	; DE - ссылка на имя директории
;
; ====================================================
;
FS_RST:	lxi h,BUF	; очищаем область буферов и переменных
	lxi b,FLEN+2-BUF
clrmem:	mvi m,0
	inx h
	dcx b
	mov a,c
	ora b
	jnz	clrmem
	STC
	LXI D,0
	MVI C,0
	CALL	FS_RDBF	; чтение в буфер нулевого сектора
	RC		; выход при ошибке чтения
	CALL	FSRST1	; проверяем метку FAT (для дисков без таблицы разделов)
	RNC
	LHLD BUF+1C6H
	XCHG
	LDA  BUF+1C8H
	MOV C,A		; (С)(DE) = номер первого сектора первого раздела
	CALL	FS_RDBF	; чтение сектора
	RC		; выход при ошибке чтения
FSRST1:	LXI H,BUF+36H	; проверка наличия метки FAT
	MOV A,M
	INX H
	CPI 'F'
	RNZ
	MOV A,M
	INX H
	CPI 'A'
	RNZ
	MOV A,M
	CPI 'T'
	STC
	RNZ		; выход, если не FAT
	LHLD BUF+0EH	; =0001	-- количество зарезервированных секторов
	LXI B,BUF+1CH	; =003F	-- количество скрытых секторов перед разделом
	LXI D,FATSEC
	CALL	ADDW2D	; вычисление FATSEC (4 байта) ?????
	LHLD BUF+16H	; =007F	-- количество секторов для одной таблицы FAT
	XCHG
	LHLD BUF+0EH	; =0001 ++
	LDA BUF+10H	; =02	-- количество таблиц FAT
	DAD D
	DCR A
	JNZ $-2		; HL = HL + DE * A
	LXI B,BUF+1CH	; =003F ++
	LXI D,ROOTSEC
	CALL	ADDW2D	; начало корневого каталога
	LHLD BUF+11H	; =0200 -- количество дескрипторов файлов в корневом каталоге
	SHLD ROOTLEN
	DAD H
	ADC A
	DAD H
	ADC A
	DAD H
	ADC A
	DAD H
	ADC A
	MOV L,H
	MOV H,A		; HL = (HL * 16)/256
	SHLD ROOTLNS	; размер корневого каталога (в секторах)
	LXI B,ROOTSEC
	LXI D,DATASEC	; первый сектор области данных
	CALL	ADDW2D	; первый сектор области данных
	LDA BUF+0DH	; =04	-- число секторов в кластере
	STA SECINBL
;	MVI A,0C3H	; =JMP
;	STA FAT_NXT
;	LDA BUF+39H	;--- FAT32
;	CPI '3'		;---
;	LXI H,FAT_N32	;---
;	JZ	FSRST2	;---
	LDA BUF+3AH
	CPI '6'
;	LXI H,FAT_N16
;	JZ	FSRST2	;--- FAT12
;	LXI H,FAT_N12	;---
	STC
	RNZ		; >> выход, не та ФС
FSRST2:;	SHLD FAT_NXT+1	; заносится адрес ПП для FAT12/16/32
	LXI H,5CH	; ="\", 00h
	SHLD CWD	; заносим в текущий путь
	XRA A
	RET
;
ADDW2D:	XRA A	; adr(DE)[4 байта] = adr(BC)[4 байта] + HL
	CALL $+3
	CALL $+3
	LDAX B
	ADC L	;+L+(c)
	STAX D
	MOV L,H
	MVI H,0
	INX B
	INX D
	RET
;
FS_RWBC:CALL	CHKBSEC	; проверка, совпадает ли содержимое буфера с номером сектора
	RZ
	LDA BUFCHG
	ANA A		; =1 -- буфер требуется сохранить
	CNZ	SAVBUF	; сохранение буфера на НЖМД
;	JMP	FS_RDBF	; и читаем новый сектор
;
; чтение сектора в буфер
;	(С)(DE) = номер сектора, помещается в BUFSEC
;	BUF -- буфер для чтения
;
FS_RDBF:MVI B,00h	; << чтение сектора в буфер
	LXI H,BUFSEC
	MOV M,E
	INX H
	MOV M,D
	INX H
	MOV M,C		; сохранение номера сектора
	LXI H,BUF
	JMP	L_RWHD	; => IDE
;
CHKBSEC:LXI H,BUFSEC	; проверка номера сектора
	MOV A,M
	CMP E
	RNZ
	INX H
	MOV A,M
	CMP D
	RNZ
	INX H
	MOV A,M
	CMP C
	RET
;
SAVBUF:	PUSH D		; сбрасываем буфер на НЖМД
	PUSH B
	LXI H,BUFSEC
	MOV E,M
	INX H
	MOV D,M
	INX H
	MOV C,M		; чтение номера сектора
	MVI B,0FFh	; << запись сектора из буфера
	LXI H,BUF
	XRA A
	STA BUFCHG	; снятие признака "буфер изменён"
	CALL	L_RWHD	; => IDE
	POP B
	POP D
	RET
;
; Поиск следующего кластера по таблице FAT
; Вход:	BNEXT -- текущий кластер
; Выход:HL, BNEXT -- следующий кластер (0 = конец записи)
;
FAT_N16:;LXI H,BNEXT+1	; номер кластера файла /256
	;CALL	LD_D	; >> [BC][DE] = adr(HL)[4 байта] -- для FAT16 достаточно 2 байта
	;LXI H,FATSEC
	;CALL	ADD_DR	; >> [BC][DE] = [BC][DE] + adr(HL)
	LDA BNEXT+1	; номер следующего кластера файла /256
	MOV E,A
	MVI D,0
	LHLD FATSEC
	DAD D
	XCHG
	LDA FATSEC+2
	MOV C,A		; [C][DE] = FATSEC + [номер кластера FAT]/256
;	MVI B,0
	CALL	CHKBSEC	; проверка, совпадает ли содержимое буфера с номером сектора
	CNZ	FS_RDBF	; >> чтение FAT-сектора в буфер
	JC      FAT_EOF	; >> не прочиталось -- обнуление BNEXT и ret
	LHLD BNEXT	; номер текущего кластера файла
	MVI H,0
	DAD H		; HL = (HL % 256)*2
	LXI D,BUF
	DAD D		; HL = позиция записи в FAT
	MOV E,M
	INX H
	MOV D,M		; DE = номер следующего кластера
	MOV A,E
	ORI 7		; 0000 0111
	ANA D
	INR A		; номер кластера >= FFF8h? (до FFFFh)
	JZ      FAT_EOF	; >> обнуление BNEXT и ret
	XCHG
	SHLD BNEXT	; номер следующего кластера файла
	RET
;
; Запись текущего кластера в таблицу FAT и поиск следующего
; Вход:	DE -- текущий кластер, HL -- чем затирать
; Выход:DE -- следующий кластер, Z=0 -- конец записи
;	+BC -- перезаписанный (текущий) кластер
;
FAT_SET:PUSH H
	PUSH D
	MOV E,D
	MVI D,0		; номер кластера файла /256
	LHLD FATSEC
	DAD D
	XCHG
	LDA FATSEC+2
	MOV C,A		; [C][DE] = FATSEC + [номер кластера FAT]/256
	CALL	FS_RWBC	; >> чтение FAT-сектора в буфер +++++++ с предв.сохр.
	POP H		; номер текущего кластера файла
	POP B		; чем затирать
	RC      	; >> не прочиталось -- ret
	MVI H,0
	DAD H		; HL = (HL % 256)*2
	LXI D,BUF
	DAD D		; HL = позиция записи в FAT
	MOV E,M
	MOV M,C		; записываем...
	INX H
	MOV D,M		; DE = номер следующего кластера
	MOV M,B		; записываем...
	MVI A,1
	STA BUFCHG	; уст. признак "буфер изменён"
	MOV A,E
	ORI 7		; 0000 0111
	ANA D
	INR A		; номер кластера >= FFF8h? (до FFFFh)
	RET
;
;  поиск файла (не директории и не метки тома)
;; DE - имя/маска искомого файла, выход: HL - ссылка на строку записи FAT
;
FS_FNFF:CALL	FS_FNDF
FNFF0:	RC		; >>> файл не найден
	PUSH H
	MOV  A, L
	ADI  0Bh
	MOV  L, A
	JNC	$+4
	INR  H		; (если вдруг был перенос)
	MOV  A, M	; читаем следующий байт после имени файла
	POP  H
	ANI  018H
	RZ		; >> не директория и не метка тома, выход
FS_FNFN:CALL	FS_FNDN	; поиск следующей записи
	JMP	FNFF0
;
;; DE - имя/маска искомого файла/директории, выход: HL - ссылка на строку записи FAT
;
FS_FNDF:PUSH D
	LDA CWD+1	; второй символ строки текущего пути
	ORA A
	JNZ FS_FF1	; -> не корень ФС
	CALL	FAT_EOF	; >> обнуление BNEXT
	LXI D,ROOTSEC
	LXI H,BLKSEC
	CALL	COPY4	; >> копировать из adr(ROOTSEC) в adr(BLKSEC) 4 байта
	LDA ROOTLNS
	JMP	FS_FF2
;
FS_FF1:	LXI D,CWDBLK	; номер кластера текущей директории
	LXI H,BNEXT	; номер следующего кластера
	CALL	COPY4	; >> копировать из adr(DE) в adr(HL) 4 байта
	XRA A
FS_FF2:	STA BLKLEN	; осталось прочитать секторов в кластере
	XRA A
	STA DIRREC
	POP D
FS_FNDN:PUSH D		; <<< искать дальше, в DE ссылка на маску файлов
	CALL	FS_NEXT	; >>
	POP D
	RC		; >> при ошибке чтения
	PUSH H
	MVI C,20H
	XRA A
	ORA M		; <-(1)
	INX H
	DCR C
	JNZ $-3		; ->(1)
	POP H
	ORA A
	STC
	RZ		; проверка на пустую запись (не нули?)
	MOV A,M
	CPI 0E5H	; признак удаления записи
	JZ      FS_FNDN	; повтор, если удалено
	PUSH D
	PUSH H
	CALL	STRMASK	; >> проверка имени файла по маске
	POP H
 	POP D
	RZ		; >> всё отлично, возврат
	JMP	FS_FNDN	; маска файла не прошла, повтор поиска
;
FAT_EOF:XRA A		; обнуление BNEXT
	MOV L,A
	MOV H,A
	SHLD BNEXT	; номер следующего кластера файла
	SHLD BNEXT+2
	RET
;
COPY4:	LXI B,4		; копировать из adr(DE) в adr(HL) 4 байта
COPY:	MOV A,C
	ORA A
	JZ	COPY1
	INR B
COPY1:	LDAX D
	MOV M,A
	INX D
	INX H
	DCR C
	JNZ	COPY1
	DCR B
	JNZ	COPY1
	RET
;
FS_NEXT:LDA DIRREC
	ORA A
	JNZ	FSNX1	; -> если DIRREC <> 0, то пропускаем загрузку
;	LXI H,BUFDIR
;	CALL	FS_READ	; >> читать в BUFDIR
;	RC
	CALL	FS_RDWR	; вычисление номера сектора для чтения
	LXI H,DIRSEC	; сохраняем номер сектора с директорией этого файла
	MOV M,E
	INX H
	MOV M,D
	INX H
	MOV M,C
	LXI H,BUFDIR
	MVI B,00h	; чтение
	CALL	L_RWHD	; >>> IDE читать в BUFDIR
	RC
	XRA A 
FSNX1:	MOV L,A
	INR A
	CPI 10H
	JC $+4		; если >=10h, то обнулить DIRREC
	XRA A
	STA DIRREC	; сохраняем номер следующей записи
FSNFP:	MVI H,0		; HL= [00][DIRREC]
	DAD H
	DAD H
	DAD H
	DAD H
	DAD H
	LXI D,BUFDIR
	DAD D		; HL = HL * 32 + BUFDIR
	RET
;
STRMASK:MVI C,11
STRMS1:	CALL	STRMSLD	; >>
	ORA A
	JZ	STRMS5
	CPI '.'
	JZ	STRMS4
	CPI '*'
	JZ	STRMS6
	CPI '?'
	JZ	STRMS3	; пропускаем сравнивание
STRMS2:	CMP M
	RNZ
STRMS3:	INX D
	INX H
	DCR C
	JNZ	STRMS1
	RET
;
STRMS4:	MOV A,C
	CPI 11
	JZ	STRMS9
	CPI 4
	JC	STRMS5a
	JNZ	STRMS5
	INX D
STRMS5:	MVI A,20H
	DCX D
	JMP	STRMS2
;
STRMS5a:INX D
	JMP	STRMS1
;
STRMS6:	INX D
	CALL	STRMSLD	; >>
	ORA A
	RZ		; конец строки
	CPI ' '
	RZ		; строка может заканчиваться и пробелом
	MVI B,10H
	CPI '.'
	JNZ	STRMS7
	MOV A,C
	SUI 2
	MOV B,A
	MVI A,20H
STRMS7:	DCR B
	JZ	STRMS5a
	CMP M
	JZ	STRMS8
	INX H
	DCR C
	JNZ	STRMS7
	ORA A
	RET
;
STRMS8:	LDAX D
	CPI '.'
	JZ	STRMS1
	JMP	STRMS3
;
STRMS9:	LDAX D
	ORA A
	JNZ $+6
	DCX D
	MVI A,20H
	CMP M
	RNZ
	INX D
	INX H
	DCR C
	JNZ	STRMS9
	LDAX D
	ORA A
	RET
;
STRMSLD:LDAX D
	CPI 'a'
	RC
	CPI 'z'+1
	RNC
	ANI 0DFH
	RET
;
;; HL - addr for store readed data
;
FS_READ:CALL	FS_RDWR
	MVI B,00h	; чтение
	JMP	L_RWHD
;
FS_WRITE:CALL	FS_RDWR
	MVI B,0FFh	; запись
	JMP	L_RWHD
;
; Вход:	BLKLEN	= осталось прочитать секторов в кластере, если <>0, то
;	BLKSEC	= номер сектора для чтения/записи
; иначе	BNEXT	= номер кластера для чтения/записи
;
; Выход:(C)(DE) = номер сектора для чтения/записи
;	BLKSEC	= номер следующего сектора для чтения
;	BLKLEN	= осталось прочитать секторов в кластере, если =0, то
;	BNEXT	= номер следующего кластера для чтения/записи
;
FS_RDWR:PUSH H		; вычисление номера сектора, откуда читать или куда писать
	LDA BLKLEN	; осталось прочитать секторов в кластере
	ORA A
	JNZ	FSRD2	; >> нет перехода к следующему кластеру
	LXI H,BNEXT	; номер следующего кластера файла
	CALL	CMPD0	; >> проверка, равно ли BNEXT нулю? (4 байта)
	JNZ	FSRD1	; >> следующий кластер
	POP H
	STC
	RET
;
FSRD1:	LXI D,DATASEC	; первый сектор области данных
	LXI H,BLKSEC
	CALL	COPY4	; >> копировать из adr(DE) в adr(HL) 4 байта
	LXI B,2
	LXI H,BNEXT	; номер следующего кластера файла
	MOV A,M
	SUB C
	MOV E,A
	INX H
	MOV A,M
	SBB B
	MOV D,A
	INX H
	MOV A,M
	SBB B
	MOV C,A
	INX H
	MOV A,M
	SBB B
	MOV B,A		; (BC)(DE)=BNEXT-2 (4 байта)
	LDA SECINBL	; число секторов в кластере
	LXI H,BLKSEC
	CALL	MULDB	; >> BLKSEC= сектор чтения/записи {DATASEC + (BNEXT-2)*SECINBL}
	CALL	FAT_N16	; !!! ;CALL	 FAT_NXT	; >>>>>> PVV only fat16
	LDA SECINBL	; число секторов в кластере
FSRD2:	DCR A
	STA BLKLEN	; осталось прочитать секторов в кластере
	LXI H,BLKSEC
	MOV E,M
	INR M		; номер сектора +1 (в BLKSEC)
	INX H
	MOV D,M
	JNZ $+4
	INR M
	INX H
	MOV C,M		; читаем номер сектора из BLKSEC в (C)(DE)
	JNZ $+4
	INR M
	POP H
	RET
;
CMPD0:	MOV A,M
	INX H
	ORA M
	INX H
	ORA M
	INX H
	ORA M
	RET
;
SHL_D:	MOV A,E
	ADD A
	MOV E,A
	MOV A,D
	ADC A
	MOV D,A
	MOV A,C
	ADC A
	MOV C,A
	MOV A,B
	ADC A
	MOV B,A
	RET
;
ADD_D:	MOV A,M
	ADD E
	MOV M,A
	INX H
	MOV A,M
	ADC D
	MOV M,A
	INX H
	MOV A,M
	ADC C
	MOV M,A
	INX H
	MOV A,M
	ADC B
	MOV M,A
	RET
;
MULDB:	CALL $+3	; adr(HL)[4б]= adr(HL)[4б] + [(BC)(DE)] * A
	CALL $+3
	CALL $+3
	RRC
	PUSH PSW
	JNC	MULDB1
	PUSH H
	CALL	ADD_D	; >>
	POP H
MULDB1:	CALL	SHL_D	; >> (можно перенести сюда)
	POP PSW
	RET
;
; DE - ссылка на имя директории
;
FS_CHDIR:LDAX D
	SUI 5CH		; ="\"
	JNZ	FS_CD1
	STA CWD+1
	INX D
	LDAX D
	ORA A
	RZ		; переход в корень, если после "\" идёт 00h
FS_CD1:	PUSH D
FS_CD2:	LDAX D
	ORA A
	JZ	FS_CD3	; >> конец строки
	INX D
	SUI 5CH		; ="\"
	JNZ	FS_CD2	; ищем конец строки или "\"
	DCX D
	STAX D
	INX D		; отрезаем всё лишнее
FS_CD3:	XCHG		; HL = конец строки
	POP D		; DE = начало
	PUSH H
	PUSH D
	CALL	FS_FNDF	; ищем в списке файлов
	POP B
	JC	FS_CD4	; ошибка поиска
	PUSH H
	LXI D,11
	DAD D
	MOV A,M
	POP H
	RLC
	RLC
	RLC
	RLC
	CMC
FS_CD4:	POP D
	RC
	PUSH D
	PUSH B
	CALL	FS_GETB	; >>
	LXI H,CWDBLK	; номер сектора текущей директории
	MOV M,E
	INX H
	MOV M,D
	INX H
	MOV M,C
	INX H
	MOV M,B		; сохраняем.
	POP D
	LXI H,CWD
	MOV A,M
	INX H		; <-(1)
	MOV B,A
	MOV A,M
	ORA A
	JNZ $-4		; (1)->
	LDAX D
	CPI '.'
	JNZ	FS_CD5
	INX D
	LDAX D
	CPI '.'
	JNZ	FS_CD6
	DCX H		; <-(2)
	MOV A,M
	MVI M,0
	CPI 5CH		; = "\"
	JNZ $-6		; (2)->
	STA CWD
	JMP	FS_CD6
;
FS_CD5:	MOV A,B
	CPI 5CH
	JZ $+6		; ->(3)
	MVI M,5CH
	INX H		; <-(3)
	CALL	STRCPY	; >>
FS_CD6:	POP D
	LDAX D
	ORA A
	JNZ	FS_CD1
	RET
;
FS_GETB:LXI D,20
	DAD D
	MOV C,M
	INX H
	MOV B,M
	LXI D,5
	DAD D
	MOV E,M
	INX H
	MOV D,M
	RET
;
STRCPY:	LDAX D
	MOV M,A
	ORA A
	RZ
	INX D
	INX H
	JMP	STRCPY
;
;Открыть следующий файл
;Вход:	FNAMES	= имя предыдущего файла в директории
;	+ всё, что в F_OPEN
F_OPNN:	CALL	L_FRCL	; обнуляем запись параметров файла
	PUSH D
	LXI  D, FNAMES	; строка 12 байт, имя предыдущего файла в формате "8.3"
	CALL	FS_FNFF	; поиск текущего файла
	POP  D		; маска поиска
	CALL	FS_FNFN	; поиск следующего файла
	RC		; >>> файл не найден
	JMP	FS_OPEN	; >> открываем
;
;Открытие файла
;Вход:	DE = ссылка на имя файла/маску для открытия (строка),
;	в случае маски открывается первый найденный файл/директория
;Выход:	признак C==1 если файл не найден
;	FLENH,FLEN	= размер файла из ФС
;	DIRSEC		= номер сектора с директорией файла
;	BNEXT		= номер первого кластера для чтения/записи
;	FNAME		= имя файла в директории (формат "11 байт")
;	FNAMES		= имя файла в директории (формат "8.3")
F_OPEN:	CALL	L_FRCL	; обнуляем запись параметров файла
	CALL	FS_FNFF	; поиск файла
	RC		; >>> файл не найден
;
FS_OPEN:PUSH H		; << открытие объекта. Предварительно требуется сделать FS_FNDF/FS_FNDN
	CALL	FS_GETB
	LXI  H,	BNEXT	; номер следующего кластера файла
	MOV  M, E
	INX  H
	MOV  M, D
	INX  H
	MOV  M, C
	INX  H
	MOV  M, B
	XRA  A
	STA	BLKLEN	; осталось прочитать секторов в кластере
	POP  B		; откуда
	LXI  D,	FNAME	; куда
	LXI  H,	FNAMES	; + строка 13 байт для FNAME в формате "8.3"
	MVI  A, 8	; счётчик
	CALL	L_NFCP	; копирование ИФ
	MVI  M, '.'
	INX  H
	MVI  A, 3
	CALL	L_NFCP	; копирование расширения
	MVI  M, 0	; -- циклы переноса ИФ в FNAME и FNAMES
	LDAX B		; читаем следующий байт после имени файла
	ani  018H
	RNZ		;FSO1	; >> это директория или метка тома, пропускаем
	LXI  H, 00014h
	DAD  B		; HL = адрес размера файла
	mov  A, M
	STA	FLENH+1
	dcx  H
	mov  A, M
	STA	FLENH
	dcx  H
	mov  A, M
	STA	FLEN+1
	dcx  H
	mov  A, M
	STA	FLEN	; сохраняем размер файла из ФС
FSO1:	RET
;
L_FRCL:	LXI  H, 0	; очистка записи параметров файла
	SHLD	FBLEN
	SHLD	FLEN	; обнуляем запись о длине файла
	SHLD	FLENH
	SHLD	FNAME	; обнуляем начало имени файла
	RET
;
L_NFCP:	PUSH PSW
	LDAX B		; копирование ИФ
	INX  B
	STAX D
	INX  D
	CPI  ' '
	JZ	L_NFC1	; пропускаем пробелы
	MOV  M, A
	INX  H
L_NFC1:	POP  PSW
	DCR  A
	JNZ	L_NFCP
	RET
;
; HL - addr for store readed data, BC - size of readed data
;
F_READ:	MOV A,B
	ORA C
	RZ	; размер =0, возврат
	PUSH H
	LHLD FBLEN
	MOV A,H
	ORA L
	JZ	F_RD1
	CALL	SUBHB	; HL = HL - BC
	JNC	F_RD3
	POP D
	PUSH B
	LHLD FBLEN
	MOV B,H
	MOV C,L
	POP H
	CALL	SUBHB	; HL = HL - BC
	PUSH H
	LHLD FBPOS
	XCHG
	CALL	COPY	; >>
	POP B
	PUSH H
F_RD1:	MOV A,B
	CPI 2
	JNC	F_RD2
	PUSH B
	LXI H,200H
	SHLD FBLEN
	LXI H,BUFDIR
	SHLD FBPOS
	CALL	FS_READ	; >> читать в BUFDIR
	POP B
	POP H
	JNC	F_READ	; цикл
	RET
;
F_RD2:	LXI H,0
	SHLD FBLEN
	POP H
	PUSH B
	CALL	FS_READ	; >> читать в adr(HL) ???
	POP B
	RC
	DCR B
	DCR B
	JMP	F_READ
;
F_RD3:	SHLD FBLEN
	LHLD FBPOS
	XCHG
	POP H
	CALL	COPY	; >>
	XCHG
	SHLD FBPOS
	XCHG
	RET
;
SUBHB:	MOV A,L		; HL = HL - BC
	SUB C
	MOV L,A
	MOV A,H
	SBB B
	MOV H,A
	RET
;
; HL - адрес начала данных, BC - размер блока данных для записи в файл
;
F_WRITE:
; проверка размера
	PUSH H		; сохраняем адрес начала данных в стек
	PUSH B		; размер записи в байтах
	CALL	BT2CL	; переводим в кластеры
	MOV D,A
	LHLD FLEN	; размер открытого файла в байтах
	MOV C,L		;PUSH H
	MOV B,H		;POP B		; BC = HL
	CALL	BT2CL	; переводим в кластеры
	POP B		; восстанавливаем BC
	SUB D		; корректировка (-X -- дописать, 0 -- совпало, +Х -- обрезать)
	JZ	F_WRD	; >> исправлять FAT не надо
	LHLD BNEXT	; читаем номер первого кластера файла
	PUSH H		; сохраняем в стек для отката
	PUSH B
	JNC	F_WRU	; >> надо освободить кластеры
;-------------------------------------------------
; корректировка таблицы FAT -- добавление кластеров
	LHLD BNEXT	; читаем номер текущего кластера файла
	STA CORLEN	; сколько дописать кластеров
F_WRA1:	PUSH H		; сохраняем в стек
	CALL	FAT_N16	; найти следующий кластер
	POP D		; (номер кластера)
	JNZ	F_WRA1	; повторять до последнего
	PUSH D
	XRA A
	STA BUFCHG	; снятие признака "буфер изменён"
F_WRA2:	CALL	FNDCL	; ищем свободный кластер (сохранение на второй проход и далее)
	POP D		; последний кластер
	PUSH H		; следующий кластер
	CALL	FAT_SET	; записываем в FAT
	MOV D,B		; D = старший разряд очередного кластера, остальное не важно...
	LXI H,CORLEN
	INR M
	JNZ	F_WRA2	; цикл
	POP D
	JMP	F_WRU4	; >> 
;
; -- уменьшение количества кластеров
F_WRU:;	LHLD BNEXT	; читаем номер первого кластера файла
;	PUSH H		; сохраняем в стек для отката
;	PUSH B
F_WRU1:	DCR D		; кластеры -1
	JZ	F_WRU2	; отсчитываем кластеры до предпоследнего
	PUSH D
	CALL	FAT_N16
	POP D
	JMP	F_WRU1	; проходим в FAT по кластерам
;
F_WRU2:	LHLD BNEXT	; читаем номер кластера файла
	XCHG		; DE = номер кластера
	LXI H,0FFFFh	; последний помечаем FFFFh
F_WRU3:	CALL	FAT_SET	; очищаем записи далее с записью FAT
	JZ	F_WRU4
	LXI H,0		; остальное затираем нулями
	JMP	F_WRU3	; >> ищем следующий кластер
;
F_WRU4:	CALL	SAVBUF	; сбрасываем буфер FAT на НЖМД
	POP B
	POP H
	SHLD BNEXT	; восстанавливаем номер первого кластера файла
	JMP	F_WRY	; корректировка размера файла в директории
;
;-------------------------------------------------
; коррекировка размера файла в директории
F_WRD:;	LHLD FLEN	; размер открытого файла в байтах
	MOV A,C
	SUB L
	JNZ	F_WRY	; пишем новый размер
	MOV A,B
	SBB H
	JZ	F_WRZ	; размер данных совпадает с размером файла (A=0)
F_WRY:	PUSH B
	LDA DIRREC
	DCR A
	ANI 0Fh		; если DIRREC=0, то делаем F
	MOV L,A
	CALL	FSNFP	; вычисление адреса записи в буфере по номеру
	LXI D,001Ch	; смещение на значение размера файла
	DAD D
	MOV M,C
	INX H
	MOV M,B		; записываем новый размер файла в буфер директории
	LXI H,DIRSEC	; << запись номера сектора директории
	MOV E,M
	INX H
	MOV D,M
	INX H
	MOV C,M
	MVI B,0FFh	; запись
	LXI H,BUFDIR
	CALL	L_RWHD	; => IDE
	POP B
F_WRZ:	POP H		; восстанавливаем HL (адрес начала данных)
;-------------------------------------------------
F_WR0:	MOV A,B
	ORA C
	RZ		; размер записи =0, возврат
	PUSH H
	LHLD FBLEN
	MOV A,H
	ORA L
	JZ	F_WR1	; >>
	POP H
	RET
;
F_WR1:	MOV A,B
	CPI 2
	JNC	F_WR2	; >> писать не меньше 1 сектора
	LXI H,200H	; = 1 сектор
	SHLD FBLEN
	POP H
	PUSH B
	CALL	FS_WRITE	; >>
	POP B
	JNC	F_WR0
	RET		; >>> выход по ошибке записи
;
F_WR2:	LXI H,0		; << пишем 1 сектор
	SHLD FBLEN
	POP H
	PUSH B
	CALL	FS_WRITE	; >>
	POP B
	RC		; >>> выход по ошибке записи
	DCR B
	DCR B		; адрес записи - 512
	JMP	F_WR0
;
; перевод размера в байтах в число занимаемых кластеров
; Вход: BC -- размер
; Выход: A -- количество кластеров
BT2CL:	DCX B		; -1 на случай совпадения размера с целым числом кластеров
	LDA SECINBL	; A = 0000 0100
	MOV C,A		; C больше не нужен, сохраняем там A
	DCR A
	STC
	RAL		; A = 0000 0111
	ORA B
	INR A		; размер округлён в большую сторону до целого числа кластеров
B_DIV:	RRC
	MOV B,A
	MOV A,C
	RRC
	MOV C,A
	MOV A,B
	JNC	B_DIV	; делим на число секторов в кластере
	RNZ
	INR A		; +1 если получилось кластеров = 0 (при размере = 0)
	RET
;
; поиск свободного кластера и пометка его FFFFh
; Вход:	 D -- старший разряд номера кластера (с которого начать поиск)
;	 FATSEC -- номер первого сектора таблицы FAT
;	 BUFCHG -- признак необходимости предварительно сохранить буфер
; Выход: HL= номер свободного кластера
;	 + сектор FAT в буфере BUF, где было найдено
FNDCL:	MOV B,D		; (счётчик секторов FAT)
	LHLD FATSEC
	MOV A,L
	ADD B
	MOV E,A
	MOV A,H
	ACI 0
	MOV D,A
	LDA FATSEC+2
	ACI 0
	MOV C,A		; [C][DE] = FATSEC + D
FNC01:	PUSH B
	PUSH D
	CALL	FS_RWBC	; чтение FAT-сектора в буфер (с предв.охранением)
	JC	FNC05	; >> не прочиталось...
	DI
	LXI H,0
	DAD SP
	SHLD FNCSP+1	; сохраняем указатель стека
	LXI SP,BUF
	MVI C,0		; счётчик записей в FAT
FNC02:	POP H
	MOV A,L
	ORA H
	JZ	FNC03	; нашли...
	INR C
	JNZ	FNC02	; цикл, ищем 0000
; переход к следующему сектору FAT
	ANA A		; сбрасываем Z
	JMP	FNCSP
;
FNC03:	LXI H,0FFFFh
	PUSH H		; помечаем кластер, как занятый (буфер пока не сохраняем!)
	MVI A,1
	STA BUFCHG	; уст. признак "буфер изменён"
FNCSP:	LXI SP,0	; сюда запишется указатель стека
	EI
	MOV L,C		; найденный номер кластера в HL (младший байт)
FNC04:	POP D
	POP B
	MOV H,B		; найденный номер кластера в HL (старший байт)
	RZ		; >> нашли -- выход
	INR B		; +1 сектор
	RZ		; >> выход на случай если нет свободных кластеров
	INR E
	JNZ	FNC01
	INR D
	JNZ	FNC01
	INR C
	JMP	FNC01	; -- читаем следующий сектор FAT
;
FNC05:	XRA A
	STC
	JMP	FNC04	; выход по ошибке, Z=1, C=1
;
