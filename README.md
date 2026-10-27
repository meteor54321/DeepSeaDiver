# 深海潛水員：水壓耗氧與減壓艙模擬器
> 1420 微算機原理及應用 — 期中個人專題  
> 平台：STM32 NUCLEO-G474RE (純暫存器開發)

## 一、 七欄規格書

| 欄位 | 規格內容 |
| :--- | :--- |
| **① 情境故事** | 潛水員下潛越深水壓越大，耗氧越快；面罩指示燈以 PWM 反映氧氣存量，過低時快閃警報；浮出水面須在減壓艙停留 3 秒呼吸燈完成減壓。 |
| **② 替身對照** | 深度計 ↔ B1 (PC13)；水下 10 分鐘 ↔ TIM3 1 秒；氧氣燈 ↔ LD2 (PA5) PWM；母船減壓艙 ↔ USART2 VCP 終端機。 |
| **③ 功能清單** | 1. 深度耗氧計算 (每 10m 耗氧率+3%/s)；2. PWM 多態調光 (漸暗/SOS快閃/呼吸燈)；3. 3 秒減壓艙保護 (輸入 surface)；4. 母船 1 秒定時回報。 |
| **④ 技術與暫存器** | W04 RCC (170MHz) / W03 GPIO (PA5 AF1, PC13 In) / W05 EXTI13 / W06 USART2 (BRR=1476) / W07 TIM2 (PWM 1kHz) & TIM3 (1ms)。 |
| **⑤ 腳位表** | PA5 (TIM2_CH1 PWM), PC13 (EXTI13 按鍵輸入), PA2/PA3 (USART2 TX/RX)。 |
| **⑥ 驗收條件** | 1. 連按 B1 三次至 40m，耗氧增為 11%/s，LD2 快速變暗；2. O2<=20% 觸發 SOS 且 LD2 0.5 秒內快閃；3. 輸入 surface 進入 3 秒減壓呼吸燈，期滿補滿 O2。 |
| **⑦ AI 揭露** | 專題故事與狀態機骨架由 AI 輔助；暫存器參數 (PSC=169, ARR=999, BRR=1476) 及 EXTI 寫 1 清除皆由人工查核對照手冊驗證。 |

DeepSeaDiver/
├── Core/                        # 專案核心原始碼目錄
│   ├── Inc/                     # 標頭檔目錄
│   │   ├── main.h               # 全域巨集、腳位定義與系統原型宣告
│   │   ├── stm32g4xx_hal_conf.h # HAL 驅動庫模組配置檔
│   │   └── stm32g4xx_it.h       # 中斷服務常式 (ISR) 宣告 (EXTI, UART, Timer)
│   ├── Src/                     # C 語言原始碼目錄
│   │   ├── main.c               # 核心邏輯 (W04 時脈/W07 PWM/狀態機/命令列解析)
│   │   ├── stm32g4xx_it.c       # 中斷處理函式本體 (EXTI15_10, USART2, TIM3)
│   │   ├── system_stm32g4xx.c   # Cortex-M4 啟動與系統初始化
│   │   └── syscalls.c           # 支援 printf 序列埠重導向底層介面
│   └── Startup/
│       └── startup_stm32g474retx.s # 向量表與組合語言啟動檔
│
├── Drivers/                     # 晶片原廠硬體抽象層與 CMSIS 核心暫存器定義
│   ├── CMSIS/                   # ARM Cortex-M4 與 STM32G4 暫存器映射定義 (RM0440)
│   └── STM32G4xx_HAL_Driver/    # 晶片原廠底層庫
│
├── DeepSeaDiver.ioc             # STM32CubeMX 圖形化引腳與時脈組態設定檔
└── README.md                    # 專案說明文件（含期中評分《七欄規格書》與驗收指標）
