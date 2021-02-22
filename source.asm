
		mode new

		pc <- start

start		r0 <- $FF00
		r0 = r0 ^ r0
		r1 <- r0 - 1
		dw 0