		mode new
		pc <- start
.top
start		r0 <- $FF00
		r0 = r0 ^ r0
.local		r1 <- r0 - 1
		void = r0 - r3
		dw 0