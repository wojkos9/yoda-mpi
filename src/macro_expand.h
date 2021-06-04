#ifndef __MACRO_EXPAND_H__
#define __MACRO_EXPAND_H__

#define ADC(...) __VA_ARGS__ 
#define EXPAND(x) ADC(x, )
#define QUOTE_EXPAND(x) ADC(#x, )
#define PREFIX_EXPAND(PREF, x) ADC(ST_##x, )

#define MAC(x) EXPAND_FUN(x) MAC2
#define MAC2(x) EXPAND_FUN(x) MAC

#define MAC_END
#define MAC2_END


#define CAT0(x, ...) __VA_ARGS__ ## x
#define CAT(x, ...) CAT0(x, __VA_ARGS__)

#define FUN_APPLY(x) CAT(_END, MAC x)

#endif // __MACRO_EXPAND_H__
