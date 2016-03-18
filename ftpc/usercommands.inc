cmd_help:

	push	str_help
	call	[con_write_asciiz]

	jmp	wait_for_usercommand


cmd_bye:

; Send BYE message to the server
	mov	dword[buf_cmd], "BYE" + 13 shl 24
	mov	byte[buf_cmd+4], 10
	mcall	send, [controlsocket], buf_cmd, 5, 0

; Close the control connection
	mcall	close, [controlsocket]
	jmp	main


cmd_pwd:

	mov	dword[buf_cmd], "PWD" + 13 shl 24
	mov	byte[buf_cmd+4], 10
	mcall	send, [controlsocket], buf_cmd, 5, 0

	jmp	wait_for_servercommand


cmd_cwd:

	mov	dword[buf_cmd], "CWD "

	mov	ecx, 256
	xor	al, al
	mov	edi, buf_cmd
	repne scasb
	lea	esi, [edi - buf_cmd]
	mov	word [edi - 2], 0x0a0d

	mcall	send, [controlsocket], buf_cmd, , 0

	jmp	wait_for_servercommand


cmd_dele:

	mov	dword[buf_cmd], "DELE"
	mov	byte[buf_cmd+4], " "

	mov	ecx, 256
	xor	al, al
	mov	edi, buf_cmd
	repne scasb
	lea	esi, [edi - buf_cmd]
	mov	word [edi - 2], 0x0a0d

	mcall	send, [controlsocket], buf_cmd, , 0

	jmp	wait_for_servercommand


cmd_list:
	call	open_dataconnection

	mov	[operation], OPERATION_LIST

	mov	dword[buf_cmd], "LIST"
	mov	word[buf_cmd+4], 0x0a0d
	mcall	send, [controlsocket], buf_cmd, 6, 0

	jmp	wait_for_servercommand


cmd_retr:      ; execution depends on is_rest flag.

	call	open_dataconnection

; Create/open the file
	cmp	byte[is_rest],1
	jne	.buf_changed
	call	copy_to_cmd ;return correct buffer to retrieve file
	;invoke  con_write_asciiz, buf_cmd
	;int 3
	.buf_changed:
	mov	eax,dword[is_rest]
	cmp	eax,0
	jne	 .rest_part

	mov	esi, buf_cmd+5
	mov	ecx, 256-5
	call	set_filename
	mov	[filestruct.subfn], 2	; create/rewrite file
	mov	[filestruct.offset], 0
	mov	[filestruct.offset+4], 0
	mov	[filestruct.size], 0
	mov	[filestruct.ptr], 0
	mcall	70, filestruct
	cmp	eax, -1
;        je      fileerror

; Prepare to write to the file
	.rest_part:
	mov	eax,[size_check]
	mov	[filestruct.offset],eax
	mov	[filestruct.subfn], 3	; write to file
	mov	[operation], OPERATION_RETR

; Request the file from server

	mov	dword[buf_cmd], "RETR"
	mov	byte[buf_cmd+4], " "
	mov	ecx, 256
	xor	al, al
	mov	edi, buf_cmd
	repne scasb
	lea	esi, [edi - buf_cmd]
	mov	dword[edi - 2], 0x0a0d
	mov	[is_rest],0
	mcall	send, [controlsocket], buf_cmd, , 0

	invoke	con_write_asciiz, buf_cmd     ; print command
	jmp	wait_for_servercommand

cmd_rdir:

	mov	[operation], OPERATION_RDIR

; Request filename list from the server

	call	open_dataconnection

	mov	[ptr_fname], 0
	mov	[size_fname], 0
	mov	dword[buf_cmd], "NLST"
	mov	word[buf_cmd+4], 0x0a0d
	mcall	send, [controlsocket], buf_cmd, 6, 0

	jmp	wait_for_servercommand

cmd_stor:

	call	open_dataconnection

	mov	[operation], OPERATION_STOR

	mov	[filestruct.subfn], 0	; read file
	mov	[filestruct.offset], 0
	mov	[filestruct.offset+4], 0
	mov	[filestruct.size], BUFFERSIZE
	mov	[filestruct.ptr], buf_buffer2

	mov	esi, buf_cmd+5
	mov	ecx, 256-5
	call	set_filename

	mov	dword[buf_cmd], "STOR"
	mov	byte[buf_cmd+4], " "

	mov	ecx, 256
	xor	al, al
	mov	edi, buf_cmd
	repne scasb
	lea	esi, [edi - buf_cmd]
	mov	word [edi - 2], 0x0a0d
	mcall	send, [controlsocket], buf_cmd, , 0

	jmp	wait_for_servercommand


cmd_lcwd:

	mov	esi, buf_cmd+5
	cmp	byte[esi], 10
	je	.print
	mov	ecx, 256-5
  .loop:
	lodsb
	cmp	al, 10
	je	.done
	test	al, al
	je	.done
	loop	.loop
  .done:
	mov	byte[esi-1], 0
	mcall	30, 1, buf_cmd+5	      ; set working directory
  .print:
	mcall	30, 2, buf_cmd, 256	      ; and read it again

	invoke	con_write_asciiz, str_lcwd
	invoke	con_write_asciiz, buf_cmd
	invoke	con_write_asciiz, str_newline

	jmp	wait_for_usercommand


cmd_cdup:

	mov	dword[buf_cmd], "CDUP"
	mov	word[buf_cmd+4], 0x0d0a
	mcall	send, [controlsocket], buf_cmd, 5, 0

	jmp	wait_for_servercommand


cmd_rmd:

	mov	dword[buf_cmd], "RMD "

	mov	ecx, 256
	xor	al, al
	mov	edi, buf_cmd
	repne scasb
	lea	esi, [edi - buf_cmd]
	mov	word [edi - 2], 0x0a0d

	mcall	send, [controlsocket], buf_cmd, , 0

	jmp	wait_for_servercommand


cmd_mkd:

	mov	dword[buf_cmd], "MKD "

	mov	ecx, 256
	xor	al, al
	mov	edi, buf_cmd
	repne scasb
	lea	esi, [edi - buf_cmd]
	mov	word [edi - 2], 0x0a0d

	mcall	send, [controlsocket], buf_cmd, , 0

	jmp	wait_for_servercommand

cmd_size:
	mov	dword[buf_cmd], "SIZE"
	mov	byte[buf_cmd+4], " "

	mov	ecx, 256
	xor	al, al
	mov	edi, buf_cmd
	repne scasb
	lea	esi, [edi - buf_cmd]
	mov	word [edi - 2], 0x0a0d

	mcall	send, [controlsocket], buf_cmd, , 0

	jmp	wait_for_servercommand

cmd_rest:
	mov	[is_rest],1
	mov	esi, buf_cmd+5
	mov	ecx, 256-5
	call	set_filename
	mov	[filestruct.subfn], 5	; get information about file
	mov	[filestruct.offset], 0
	mov	[filestruct.offset+4], 0
	mov	[filestruct.size], 0
	mov	[filestruct.ptr], buf_for_answer
	mcall	70, filestruct
	cmp	eax,0
	jne	.error
	mov	eax, dword[buf_for_answer+32]
	inc	eax
	mov	[size_check],eax
	;invoke  con_write_asciiz, int_string
	mov	dword[cmd_copy], "REST"
	mov	byte[cmd_copy+4], " "
	mov	edi,cmd_copy+5
	call	intToString
	add	eax,4
	mov	byte[cmd_copy+eax],10
	inc	eax
	mov	dword[cmd_copy+eax],0 ;some bad code, I don't know how to correct
	add	eax,4
	mov	dword[cmd_copy+eax],0
	add	eax,4
	mov	dword[cmd_copy+eax],0
	mov	ecx, 256
	xor	al, al
	mov	edi, cmd_copy
	repne scasb
	lea	esi, [edi - cmd_copy]
	mov	dword[edi - 2], 0x0a0d
	mcall	send, [controlsocket], cmd_copy,, 0; send rest command to server
	call	cmd_to_copy
	jmp	wait_for_servercommand
	;file is not exist
	.error:
	invoke	con_write_asciiz, str_file_error
	jmp wait_for_usercommand


; esi   = source ptr
; ecx   = max length of source buffer
set_filename:

	mov	edi, filestruct.name
  .loop:
	lodsb
	test	al, al
	jz	.done
	cmp	al, 10
	je	.done
	stosb
	loop	.loop
  .done:
	xor	al, al		; append a 0 byte
	stosb

	ret




; eax = number to stringify/output
; edi = location of buffer

intToString:
    push  edx
    push  ecx
    push  edi
    push  ebp
    mov   ebp, esp
    mov   ecx, 10

 .pushDigits:
    xor   edx, edx	  ; zero-extend eax
    div   ecx		  ; divide by 10; now edx = next digit
    add   edx, 30h	  ; decimal value + 30h => ascii digit
    push  edx		  ; push the whole dword, cause that's how x86 rolls
    test  eax, eax	  ; leading zeros suck
    jnz   .pushDigits

 .popDigits:
    pop   eax
    stosb		  ; don't write the whole dword, just the low byte
    cmp   esp, ebp	  ; if esp==ebp, we've popped all the digits
    jne   .popDigits

    xor   eax, eax	  ; add trailing nul
    stosb

    mov   eax, edi
    pop   ebp
    pop   edi
    pop   ecx
    pop   edx
    sub   eax, edi	  ; return number of bytes written
    ret

cmd_to_copy:
    push eax
    push edi
    mov edi,0
    .start:
    cmp edi,1024
    je	.end
    mov eax,dword[buf_cmd+edi]
    mov dword[cmd_copy+edi],eax
    add edi,4
    jmp .start
    .end:
    pop eax
    pop edi
    ret

copy_to_cmd:
    push eax
    push edi
    mov edi,0
    .start:
    cmp edi,1024
    je	.end
    mov eax,dword[cmd_copy+edi]
    mov dword[buf_cmd+edi],eax
    add edi,4
    jmp .start
    .end:
    pop eax
    pop edi
    ret



