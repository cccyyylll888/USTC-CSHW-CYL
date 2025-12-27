#include<stdio.h>
#include<stdlib.h>
#include<time.h>

/*
 * version5(蒙哥马利).cpp
 * 说明：此程序使用自定义大整数（BigInt）实现蒙哥马利乘法/幂运算与 Miller-Rabin 素性测试，
 *       用于生成大素数并以16进制格式输出与保存。
 * 注意：本代码中的大整数采用基数 BASE = 65536，每个元素 存储在 num[1..len]（1-based）中，
 *       低位元素 存放在 num[1]，高位在 num[len]。此处保留原有索引约定以避免改动逻辑。
 */

/*
 * 自定义大整数类型 BigInt：
 * - len: 有效元素 数量（当 len==0 表示值为 0）
 * - num: 每个元素为一个元素单元，基数为 BASE（65536）
 * 约定：num[1] 为最低有效位，num[len] 为最高有效位（小端表示）
 */
struct BigInt{
    int len;
    long long num[2000];
}; 

/* 全局变量与常量说明：
 * - R_mont: 蒙哥马利中的 R 值（按元素 表示）
 * - n_reverse: 模数的反演参数（用于蒙哥马利约简）
 * - mont_one: 蒙哥马利域中的 1（即 R mod n）
 * - PP: 暂未广泛使用的常量（保留）
 * - BASE: 大整数的基数（65536）
 * - 常用小整数 one, two, three, five, seven, eleven, thirteen, seventeen 用于小素性检查
 * - candidate: 当前生成的候选大整数
 */
BigInt R_mont, n_reverse, temp_bigint, mont_one;
long long PP = 16;
long long BASE = 65536; 
BigInt zero; 
BigInt one;
BigInt two;
BigInt three;
BigInt five;
BigInt seven;
BigInt eleven;
BigInt thirteen;
BigInt seventeen;
BigInt candidate; 
BigInt test_cnt[15]; 
/*
 * initial(): 初始化随机数种子并设置若干常用小整数的 BigInt 表示
 */
void initial()
{
    srand((unsigned)time(NULL));
    zero.len = 0;
    zero.num[1] = 0;
    one.len = 1;
    one.num[1] = 1;
    two.len = 1;
    two.num[1] = 2;
    three.len = 1;
    three.num[1] = 3;
    five.len = 1;
    five.num[1] = 5;
    seven.len = 1;
    seven.num[1] = 7;
    eleven.len = 1;
    eleven.num[1] = 11;
    thirteen.len = 1;
    thirteen.num[1] = 13;
    seventeen.len = 1;
    seventeen.num[1] = 17;
} 

/*
 * delete_zero(a): 去除大整数高位的零（修整长度）
 * 返回：修整后的 BigInt
 */
BigInt delete_zero(BigInt a)
{
    while(a.len > 0&&a.num[a.len] == 0)
    { 
        a.len--;
    }
    return a;
} 

/*
 * big(a, b): 比较两大整数 a 和 b
 * 返回：若 a > b 返回 1，否则返回 0（a <= b 返回 0）
 */
int big(BigInt a,BigInt b)
{
    if(a.len > b.len)
    {
        return 1;
    }
    if(a.len < b.len)
    {
        return 0;
    }
    for(int i = a.len;i >= 1;i--)
    {
        if(a.num[i] > b.num[i])
        {
            return 1;
        }
        if(a.num[i] < b.num[i])
        {
            return 0;
        }
    }
    return 0;
}

/* 简单辅助：返回两个 long long 的最大值 */
long long max(long long x,long long y)
{
    if(x > y)
    {
        return x;
    }
    return y;
}

/*
 * generate_number(len): 生成一个随机的大整数，长度为 len 个元素
 * 特点：最低位保证为奇数（生成可能用于素数候选）
 */
BigInt generate_number(int len)
{
    BigInt a = zero;
    a.len = len;
    int t = rand();
    a.num[1] = (rand()%(BASE/2 - 1)) * 2 + 1; // 低位为奇数
    for(int i = 2;i <= len;i++)
    {
        a.num[i] = (rand() % (BASE-1)) + 1;
    }
    a = delete_zero(a);
    return a;
}

/*
 * add_zero(a, k): 在大整数 a 的低位插入 k 个 0 元素，相当于乘以 BASE^k（左移 k 个元素）
 * 返回：扩展后的 BigInt
 */
BigInt add_zero(BigInt a,int k)
{
    BigInt tempa = zero;
    tempa.len = a.len+k;
    for(int i = k+1;i <= k+a.len;i++)
    {
        tempa.num[i] = a.num[i-k];
    }
    return tempa;
}

/*
 * print(a): 以十进制 元素 列表形式打印 BigInt（调试用）
 */
void print(BigInt a)
{
    printf("%d\n",a.len);
    for(int i = a.len;i >= 1;i--)
    {
        printf("%lld ",a.num[i]);
    }
    printf("\n");
    return; 
}

/*
 * print_hex(a): 以 16 进制打印 BigInt（每个元素 输出为 4 个16进制位），
 * 便于查看整体大整数的十六进制表示
 */
void print_hex(BigInt a)
{
    if(a.len == 0)
    {
        printf("0\n");
        return;
    }
    printf("%llx", (unsigned long long)a.num[a.len]);
    for(int i = a.len-1;i >= 1;i--)
    {
        printf("%04llx", (unsigned long long)a.num[i]);
    }
    printf("\n");
    return;
}

/*
 * plus(a, b): 大整数加法（按 BASE 进位）
 * 假设输入 a、b 的 num[] 已按索引展开；返回 a+b
 */
BigInt plus(BigInt a,BigInt b)
{
    BigInt ret = zero; 
    ret.len = max(a.len,b.len); 
    long long jinwei = 0;
    for(int i = 1;i <= ret.len;i++)
    {
        long long now = jinwei + a.num[i] + b.num[i];
        if(now >= BASE)
        {
            jinwei = 1;
        }
        else
        {
            jinwei = 0;
        }
        ret.num[i] = now % BASE;
    }
    if(jinwei > 0)
    {
        ret.len++;
        ret.num[ret.len] = jinwei;
    }
    ret = delete_zero(ret);
    return ret;
}

/*
 * minus(a, b): 大整数减法，返回 a-b（假设 a >= b）
 */
BigInt minus(BigInt a,BigInt b)
{
    BigInt ret = zero;
    ret.len = a.len;
    long long tuiwei = 0; // 借位
    for(int i = 1;i <= a.len;i++)
    {
        if(a.num[i] < b.num[i]+tuiwei)
        {
            ret.num[i] = a.num[i]+BASE-b.num[i]-tuiwei;
            tuiwei = 1;
        }
        else
        {
            ret.num[i] = a.num[i]-b.num[i]-tuiwei;
            tuiwei = 0;
        }
    }
    ret = delete_zero(ret);
    return ret;
}

/*
 * divide_two(a): 将大整数 a 右移一位（除以 2），返回结果
 */
BigInt divide_two(BigInt a)
{
    BigInt ret = zero;
    long long now = 0;
    ret.len = a.len;
    for(int i = a.len;i >= 1;i--)
    {
        if(a.num[i] % 2 == 0)
        {
            ret.num[i] = (a.num[i] + now) / 2;
            now = 0;
        }
        else
        {
            ret.num[i] = (a.num[i] + now - 1) / 2;
            now = BASE;
        }
    }
    ret = delete_zero(ret);
    return ret;
} 

/*
 * multiple(a, b): 大整数乘法（学校乘法），返回 a*b
 * 注意：此实现直接在 元素 上进行乘法并做进位处理
 */
BigInt multiple(BigInt a,BigInt b)
{ 
    BigInt ret = zero; 
    ret.len = a.len + b.len;
    for(int i = 1;i <= a.len;i++)
    {
        for(int j = 1;j <= b.len;j++)
        {
            ret.num[i+j-1] += a.num[i] * b.num[j];
            ret.num[i+j] += ret.num[i+j-1] / BASE;
            ret.num[i+j-1] %= BASE;
        }
    } 
    while(ret.num[ret.len] >= BASE)
    {
        ret.num[ret.len+1] = ret.num[ret.len] / BASE;
        ret.num[ret.len] = ret.num[ret.len] % BASE;
        ret.len++;
    }
    ret = delete_zero(ret);
    return ret;
}

/*
 * delete_end(t, k): 删除大整数 t 的低 k 个 元素（相当于整除 BASE^k）
 * 返回剩余的高位部分
 */
BigInt delete_end(BigInt t,int k)
{
    BigInt aaa = zero;
    if(t.len <= k)
    {
        return aaa;
    }
    aaa.len = t.len - k;
    for(int i = 1;i <= aaa.len;i++)
    {
        aaa.num[i] = t.num[i+k];
    }
    aaa = delete_zero(aaa);
    return aaa;
} 

/*
 * modd(a, b): 求 a mod b（使用长除法思想），返回余数
 */
BigInt modd(BigInt a,BigInt b)
{
    BigInt tempa = a;
    int k = a.len - b.len;
    while(k >= 0)
    {
        BigInt now = add_zero(b,k);
        while(!big(now,tempa))
        {
            BigInt now1 = now;
            while(!big(multiple(now1,two),tempa))
            {
                now1 = multiple(now1,two);
            }
            tempa = minus(tempa,now1);
        }
        k--;
    }
    tempa = delete_zero(tempa);
    return tempa;
} 

/*
 * find(x): 在模 65536 下寻找 x 的乘法逆元（奇数范围内线性搜索）
 * 返回：逆元 i 满足 (x*i) % 65536 == 1
 */
long long find(long long x)
{
    for(long long i = 1;i <= 65535;i += 2)
    {
        if((x*i) % 65536 == 1)
        {
            return i; 
        }
    } 
}

/*
 * reverse(): 计算用于蒙哥马利约简的 n_reverse 参数
 * 算法：基于 candidate 的最低 元素，配合 R_mont 计算反演值
 */
void reverse()
{
    long long t = candidate.num[1];
    BigInt now = zero;
    n_reverse.num[1] = find(t); 
    n_reverse.len = 1;
    now = multiple(n_reverse,candidate);
    for(int i = 2;i < R_mont.len;i++)
    {
        long long s = 65536-now.num[i];
        n_reverse.num[i] = s*n_reverse.num[1] % 65536;
        n_reverse.len++;
        now = multiple(n_reverse,candidate);
    }
    n_reverse = delete_zero(n_reverse);
    n_reverse = minus(R_mont,n_reverse);
    n_reverse = delete_zero(n_reverse);
} 

/*
 * Montgomory(a): 将 a 转换到蒙哥马利形式（a * R mod n），实现为 a * BASE^{candidate.len} mod candidate
 * 返回：转换后的 BigInt
 */
BigInt Montgomory(BigInt a)
{ 
    a = add_zero(a,candidate.len);
    a = modd(a,candidate);
    a = delete_zero(a);
    return a;
}

/*
 * Montgomory_reduce(aa): 蒙哥马利约简，将 aa 约简到标准表示（相当于 aa * R^{-1} mod n 的中间步骤）
 * 依赖全局的 n_reverse 与 candidate
 */
BigInt Montgomory_reduce(BigInt aa)
{
    BigInt a = aa; 
    BigInt m = multiple(a,n_reverse);
    for(int i = candidate.len+1;i <= m.len;i++)
    {
        m.num[i] = 0;
    } 
    if(m.len > candidate.len)
    {
        m.len = candidate.len;
    }
    a = plus(a,multiple(m,candidate)); 
    a = delete_end(a,candidate.len);
    if(big(a,candidate))
    {
        a = minus(a,candidate);
    }
    return a;
}

/*
 * qpow(a, qpow1): 在模 candidate 下使用蒙哥马利方法快速幂运算
 * - a: 底数（BigInt）
 * - qpow1: 指数（BigInt），按 元素 展开（此处按每个 元素 的 16 位分解）
 * 返回：a^{qpow1} mod candidate（在标准表示下）
 */
BigInt qpow(BigInt a,BigInt qpow1)
{  
    BigInt ans = mont_one;
    a = Montgomory(a);
    for(int i = qpow1.len;i >= 1;i--)
    {
        long long x = qpow1.num[i];
        long long y = 32768;
        for(int j = 15;j >= 0;j--)
        { 
            ans = multiple(ans,ans);
            ans = Montgomory_reduce(ans);
            if((x / y) % 2 == 1)
            {
                ans = multiple(ans,a);
                ans = Montgomory_reduce(ans);
            } 
            y /= 2;
        } 
    }
    ans = Montgomory_reduce(ans);
    ans = delete_zero(ans);
    return ans;
}

/*
 * Miller_Rabin(a): 对大整数 a 进行 Miller-Rabin 素数概率检测
 * - 首先排除能被若干小素数整除的情况（快速筛选）
 * - 使用 10 个随机基数进行检测（每个基数为随机生成的 1-元素 数），
 *   若任何一次发现复合性证据则返回 0（合数）；通过所有测试返回 1（可能为素数）
 */
int Miller_Rabin(BigInt a)
{
    if(modd(a,three).len == 0||modd(a,five).len == 0||modd(a,seven).len == 0||modd(a,eleven).len == 0||modd(a,thirteen).len == 0||modd(a,seventeen).len == 0)
    {
        return 0;
    }
    R_mont = one;
    R_mont = add_zero(R_mont,a.len);
    reverse();
    mont_one = Montgomory(one);
    for(int i = 1;i <= 10;i++)
    {
        BigInt test;
        test = generate_number(10);
        test_cnt[i] = test;
        long long k = 0;
        BigInt test1 = a;
        test1.num[1] -= 1;
        BigInt ss = test1;
        while(test1.num[1] % 2 == 0&&test1.len > 0)
        {
            k++;
            test1 = divide_two(test1); 
        }  
        test = qpow(test,test1);
        if(!big(test,one)&&!big(one,test))
        {
            continue;
        }
        if(!big(test,ss)&&!big(ss,test))
        {
            continue; 
        }
        int flag = 0;
        for(int i = 1;i <= k-1;i++)
        {
            test = qpow(test,two); 
            if(!big(test,ss)&&!big(ss,test))
            {
                flag = 1;
                break; 
            }
        }
        if(flag == 0)
        {
            return 0;
        }
    }
    return 1;
}

/*
 * main(): 程序入口
 * - 初始化常量
 * - 随机生成候选大整数并进行 Miller-Rabin 测试直到找到通过测试的数
 * - 将找到的“可能素数”以 16 进制打印并追加保存到 prime_hex.txt
 */
int main()
{
    double start = clock(); 
    initial(); 
    candidate = generate_number(64);
    double start1 = start;
    int cnt = 0;  
    while(!Miller_Rabin(candidate))
    {
        candidate = generate_number(64);
        start = clock();
    }
    print_hex(candidate);
    printf("\n���������ڲ��Ե�10����\n");
    for(int i = 1;i <= 10;i++)
    {
    	print_hex(test_cnt[i]);
	}
    // 将生成的素数以 16 进制追加保存到 prime_hex.txt
    {
        FILE *fp = fopen("prime_hex.txt", "a");
        if(fp != NULL)
        {
            if(candidate.len == 0)
            {
                fprintf(fp, "0\n");
            }
            else
            {
                fprintf(fp, "%llx", (unsigned long long)candidate.num[candidate.len]);
                for(int i = candidate.len-1;i >= 1;i--)
                {
                    fprintf(fp, "%04llx", (unsigned long long)candidate.num[i]);
                }
                fprintf(fp, "\n");
            }
            fclose(fp);
        }
    }
    return 0;
} 
