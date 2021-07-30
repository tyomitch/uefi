.code

PUBLIC SinCos
; void SinCos(double AngleInRadians, double *pSinAns, double *pCosAns);

angle_on_stack$ = 8

SinCos PROC

  movsd QWORD PTR angle_on_stack$[rsp], xmm0 ; argument angle is in xmm0, move it to the stack
  fld QWORD PTR angle_on_stack$[rsp]         ; push angle onto the FPU stack where we can do FLOPs
  fsincos
  fstp QWORD PTR [r8]  ; store/pop cosine output argument 
  fstp QWORD PTR [rdx] ; store/pop sine output argument
  ret 0

SinCos ENDP

END
