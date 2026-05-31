; ModuleID = 'twit'
source_filename = "twit"

define i32 @add(i32 %x, i32 %y) {
entry:
  %x1 = alloca i32, align 4
  store i32 %x, i32* %x1, align 4
  %y2 = alloca i32, align 4
  store i32 %y, i32* %y2, align 4
  %x3 = load i32, i32* %x1, align 4
  %y4 = load i32, i32* %y2, align 4
  %addtmp = add i32 %x3, %y4
  ret i32 %addtmp
}

define i32 @main() {
entry:
  %a = alloca i32, align 4
  store i32 10, i32* %a, align 4
  %b = alloca i32, align 4
  store i32 20, i32* %b, align 4
  %c = alloca i32, align 4
  %a1 = load i32, i32* %a, align 4
  %b2 = load i32, i32* %b, align 4
  %calltmp = call i32 @add(i32 %a1, i32 %b2)
  store i32 %calltmp, i32* %c, align 4
  ret i32 0
}
