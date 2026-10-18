# BombLab 实验报告

本报告根据本仓库的 `defuser.c`、个人 `bomb` 的反汇编记录，以及本地实际测试整理。下文把每关的 64 位参数记为 `challenge`；涉及 `eax` 等 32 位寄存器的运算均按 `uint32_t` 截断。解法输出的是满足检查的文本，不修改炸弹本身。

## 第 1 关：函数调用与参数传递

`phase_01` 先以 `valid_u32_fields(input, 1)` 检查字段数，再调用 `p1_compute(challenge)`，最后用 `cmp [rbp-0x14],eax` 比较输入和返回值。关键在于按 System V ABI 跟踪 `rdi/rsi` 参数和 `eax` 返回值。设 `lo=(uint32_t)challenge`、`hi=(uint32_t)(challenge>>32)`，由三个辅助函数得到：

```text
p1_step2: (lo + 0x8c5b) ^ hi
p1_step0: (3*lo + (uint32_t)(challenge>>16)) ^ 0x1e96
p1_step1: 5*p1_step0 ^ (p1_step2 + 0xbfc9)
```

依据是 `p1_step0` 的 `lea eax,[rdi+rdi*2]`、`p1_step1` 的 `lea eax,[rdi+rdi*4]`，以及 `p1_compute` 中的三次调用顺序。最终只输出 `p1_step1` 的结果。

## 第 2 关：循环与数组

`mov esi,0x6; call valid_u32_fields` 和 `cmp eax,0x6` 表明输入包含六个整数。`sscanf` 把它们放到连续的栈位置；随后循环用 `[rbp+rdx*4-0x40]` 取第 `rdx` 项。`cmp rdx,0x6` 是循环终点。

令 `v[0..5]` 为六个输入值。第一项由 `challenge` 的第 20 位开始的 8 位乘 3 得到；后五项遵循：

```text
v[0] = ((challenge >> 20) & 0xff) * 3
v[i] = (((challenge >> (24 + 4*(i-1))) & 0x3f) + i ^ v[i-1]) * 5
        (i = 1..5；先加 i，再异或 v[i-1]，最后乘 5)
```

汇编中的 `shr rax,cl` 使移位量每轮增加 4，`lea eax,[rax+rax*4]` 对应乘 5。

## 第 3 关：跳转表

主函数先验证两个整数。第一个数必须等于 `challenge & 7`；第二个数来自 `p3_dispatch`。传给分发函数的另一参数是 `(uint32_t)((challenge>>3) ^ (challenge>>32))`。

`p3_dispatch` 的 `jmp QWORD PTR [rax*8+0x405820]` 用第一个数作为下标，从表中读取**代码地址**并跳转。表项不按 `p3_case_0` 到 `p3_case_7` 的名字顺序排列，因此逐项读取目标地址、再对应各 `p3_case` 的运算。当前 `defuser.c` 用 `switch` 实现这八个分支；例如下标 0 走 `p3_case_4`，算 `5*b+0xbc76`，下标 1 走 `p3_case_0`，算 `4*b^0x20ea`。

## 第 4 关：递归二分

主函数取 `target=challenge&0x7f`、`salt=(uint32_t)((challenge>>7)^(challenge>>32))`，调用 `p4_walk(target,0,127,salt)`。在辅助函数里，`sub edi,esi; shr edi,1; add edi,esi` 得到区间中点 `mid=low+(high-low)/2`。

- `mid==target`：返回 `(mid+0x54)^salt`。
- `mid<target`：递归搜索右半区间，返回 `(3*child)^(mid+salt)^0x9`。
- `mid>target`：递归搜索左半区间，返回 `(3*child)^(mid+salt)^0x5`。

`lea eax,[rax+rax*2]` 是对子结果乘 3；`ebx` 保存当前层的 `mid+salt`。主函数把输入整数与递归返回值比较。

## 第 5 关：结构体与链表遍历

主函数检查两个整数。辅助函数 `p5_traverse` 的 `mov rdx,[rax*8+0x405b60]` 从八项**节点地址表**选起点，索引是 `challenge&7`。一个节点的布局是 `[node+0]` 的 8 字节 next 指针、`[node+8]` 的 `field8`、`[node+12]` 的 `field12`；`mov rdx,[rdx]` 沿 next 指针走到下一节点，并非跳转到代码执行。

初始累计值是 `(uint32_t)((challenge>>6)^(challenge>>32)^0xcb34)`。每处理一个节点执行 `acc=(3*acc+field12)^field8`，同时把该节点的 `field8` 写到主函数提供的地址。起点节点必处理，之后再走 `(challenge>>3)&3` 步。主函数把第一个输入与最终 `acc` 比较，把第二个输入与最后写回的 `field8` 比较。节点数据和起点对应关系记录在 `defuser.c` 的 `nodes`、`heads` 中。

## 第 6 关：函数指针表与间接调用

`p6_dispatch` 把 `challenge` 的低 3 位作为第一次函数表下标，把第 3～5 位作为第二次下标；`and ebx,0x38` 得到的是**字节偏移**，所以 `call [rbx+0x405e00]` 不会越过 8 项的函数指针表。

```text
x = (uint32_t)((challenge>>6) ^ (challenge>>32))
y = ops[challenge & 7](x)
z = ops[(challenge>>3) & 7](y ^ 0x5e66)
answer = z + 0x6618
```

`ops` 表中存在重复的函数地址，不能只按函数名顺序编号。逐项读取表中的地址后，`defuser.c` 的 `do_call` 用相同的下标映射分发。辅助函数中的 `rol`/`ror` 是 32 位循环移位，例如 `rol x,2` 对应 `(x<<2)|(x>>30)`。

## Honor：栈上复制与三字段校验

`secret_gate` 检查文本长度为 36～163 字节，再用 `secret_copy` 逐字节复制到自己的栈缓冲区。缓冲区从 `[rbp-0xd0]` 开始；`[rbp-0xb0]` 是偏移 32 的四字节标记，按小端序与 `0x4e45504f` 比较，即文本 `OPEN`。`[rbp-0xac]` 是偏移 36，传给 `secret_phase` 解析三个无符号整数。前 32 字节只作为填充。

`challenge` 是本关参数，`context=transcript_context(前六关答案)` 是由六行输出计算的 64 位摘要。在 `secret_phase` 中，`r12` 保存 `challenge`、`r13` 保存 `context`。令

```text
a = (uint32_t)context & 3
u = (uint32_t)(challenge>>32) ^ (uint32_t)challenge
v = (uint32_t)(context>>32) ^ (uint32_t)context
t = (a*0x3081) ^ (uint32_t)(context>>16)
mix(x) = ((x>>5)^0x2deb) + (x^0x3081)
b = mix(u^v) ^ t
c = b ^ (v+a+0x2deb+(u>>5))
```

程序依次比较 `a`、`b`、`c`。其中 `imul r9d,ecx,0x3081` 是乘法；`secret_mix` 内先右移、异或，再与另一项相加。求解器在输出缓冲区前 32 字节放填充字符，再写 `OPEN` 和三个十进制数，并检查 `snprintf` 是否截断。

## 验证记录

编译命令为 `make -B defuser`，公开测试命令为 `python3 public_test.py`，本地结果：第 1～6 关 **60/60**，Honor **10/10**。

另选不在 `tests/public.json` 中的 seed 17、1001、20261014，运行：

```sh
for seed in 17 1001 20261014; do
  { ./defuser --seed "$seed"; ./defuser --seed "$seed" --secret; } |
    ./bomb --seed "$seed"
done
```

三个 seed 均实际输出 `Phase 1 defused.` 至 `Phase 6 defused.`，以及 `Secret phase defused.`，进程退出码为 0。因此每个已完成关卡至少有三次公开列表外的验证。注意 `./defuser --secret` 只输出第七行，整套测试时必须先拼上前六行。

**运行截图：**课程要求截图。目前提供的终端截图显示六项检查通过，但未显示 seed 和 Honor 结果；提交前需将对应图片保存到 `images/` 并在此处插入相对路径，还需补充显示非公开 seed 和 `Secret phase defused.` 的截图。这里不把文字测试记录冒充截图。

## 资料与工具使用

- 本仓库 `README.md`：接口、测试和提交要求；`asm/` 下的反汇编记录：各关关键指令。
- 本地工具：`objdump`、GDB、`make`、`public_test.py`、个人 `bomb`；上述验证结果来自实际运行。
- Codex 辅助解释汇编、核对 C 代码和测试，并补全了 Honor 的 `solve_secret`、整理本报告。报告中的关系以当前 `defuser.c` 和个人炸弹的实际测试为准；提交前应自行复核并说明工具使用情况。
