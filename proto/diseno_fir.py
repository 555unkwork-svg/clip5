import numpy as np
def ls_fir(ntaps, fs, fpass, fstop, target, D, wstop=1e4, ngrid=4000):
    w=np.linspace(0,np.pi,ngrid); f=w*fs/(2*np.pi)
    W=np.where(f<=fpass,1.0,np.where(f>=fstop,wstop,0.0))
    T=np.where(f<=fpass,target(f),0.0)
    n=np.arange(ntaps)
    # minimizar sum W |sum h_n e^{-jwn} - T e^{-jwD}|^2
    C=np.cos(np.outer(w,n)); S=np.sin(np.outer(w,n))
    R=(C*W[:,None]).T@C+(S*W[:,None]).T@S
    p=(C*(W*T*np.cos(w*D))[:,None]).sum(0)+(S*(W*T*np.sin(w*D))[:,None]).sum(0)
    h=np.linalg.solve(R,p); return h
def resp(h,fs,f):
    w=2*np.pi*f/fs; return np.abs(np.exp(-1j*np.outer(w,np.arange(len(h))))@h)
fs=96000
for nt,ws in [(64,1e4),(80,1e5),(96,1e5),(96,1e6),(112,1e6),(128,1e6)]:
    D=(nt-1)/2+0.25
    comp=lambda f: 1/np.cos(np.pi*f/fs)
    h=ls_fir(nt,fs,20000,28000,comp,D,ws)
    fp=np.linspace(0,20000,500); fsb=np.linspace(28000,48000,2000)
    err=20*np.log10(resp(h,fs,fp)/comp(fp)); att=20*np.log10(resp(h,fs,fsb).max())
    # phase error -> group delay deviation
    H=np.exp(-1j*np.outer(2*np.pi*fp/fs,np.arange(nt)))@h
    ph=np.unwrap(np.angle(H))+2*np.pi*fp/fs*D
    print(nt,ws,f"rizado ±{np.abs(err).max():.3f} dB  rechazo {att:.1f} dB  error de fase máx {np.degrees(np.abs(ph).max()):.3f}°")

print("--- 1x (48 kHz) ---")
fs=48000
for nt in (24,32,40):
    D=(nt-1)/2+0.5 - 0.5  # retardo entero total: 0.5 (ADAA) + D
    D=round((nt-1)/2)-0.5
    comp=lambda f: 1/np.cos(np.pi*f/fs)
    h=ls_fir(nt,fs,19000,23500,comp,D,10)
    fp=np.linspace(0,19000,500)
    err=20*np.log10(resp(h,fs,fp)/comp(fp))
    print(nt,f"D={D} rizado ±{np.abs(err).max():.3f} dB  |H| 22k={20*np.log10(resp(h,fs,np.array([22000]))[0]):.1f} dB  ganancia máx={20*np.log10(resp(h,fs,np.linspace(0,24000,999)).max()):.1f} dB")

print("--- half-band IIR polifásico (HIIR) ---")
import math
def trans(t):
    k=math.tan((1-t*2)*math.pi/4); k*=k
    kk=(1-k*k)**0.25; e=0.5*(1-kk)/(1+kk); e2=e*e; e4=e2*e2
    return k, e*(1+e4*(2+e4*(15+150*e4)))
def coef(i,k,q,order):
    c=i+1; acc=0; j=1; ii=0
    while True:
        t=q**(ii*(ii+1))*math.sin((ii*2+1)*c*math.pi/order)*j; acc+=t; j=-j; ii+=1
        if abs(t)<1e-100: break
    num=acc*q**0.25; acc=0; j=-1; ii=1
    while True:
        t=q**(ii*ii)*math.cos(ii*2*c*math.pi/order)*j; acc+=t; j=-j; ii+=1
        if abs(t)<1e-100: break
    den=acc+0.5; ww=num/den; w2=ww*ww
    x=math.sqrt((1-w2*k)*(1-w2/k))/(1+w2); return (1-x)/(1+x)
def respiir(a,f):  # f normalizado a la tasa alta
    z=np.exp(1j*2*np.pi*f); z2=z*z
    A0=np.ones_like(z); A1=np.ones_like(z)
    for i,ai in enumerate(a):
        ap=(ai+1/z2)/(1+ai/z2)
        if i%2==0: A0=A0*ap
        else: A1=A1*ap
    return 0.5*(A0+A1/z)
for n in (8,10,12,14,16):
    k,q=trans(4/96); a=[coef(i,k,q,2*n+1) for i in range(n)]
    f=np.linspace(28/96,0.5,2000); fp=np.linspace(0,20/96,500)
    H=respiir(a,f); Hp=respiir(a,fp)
    gd=-np.gradient(np.unwrap(np.angle(respiir(a,np.linspace(1e-4,0.01,50)))),2*np.pi*np.linspace(1e-4,0.01,50))
    print(n,f"rechazo {20*np.log10(np.abs(H).max()):.1f} dB rizado {np.abs(20*np.log10(np.abs(Hp))).max():.2e} dB  retardo BF {gd.mean():.2f} muestras")
