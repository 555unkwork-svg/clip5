import numpy as np, sys
sys.path.insert(0,'.')
from diseno_fir import ls_fir
rng=np.random.default_rng(1)
def ref_tp(x, os=32):
    X=np.fft.rfft(x); n=len(x)
    Y=np.zeros(n*os//2+1,complex); Y[:len(X)]=X
    if n%2==0: Y[len(X)-1]*=0.5
    y=np.fft.irfft(Y,n*os)*os
    return np.abs(y).max()
def make_interp(taps_per_phase, fpass):
    L=4*taps_per_phase
    # interpolador a 4fs: pasa 0..fpass (rel. fs), rechaza desde 1-fpass
    h=ls_fir(L, 4.0, fpass, 1-fpass, lambda f: np.ones_like(f), (L-1)/2, 1e3)
    return h*4
def tp_est(x,h,parab):
    u=np.zeros(len(x)*4); u[::4]=x
    z=np.convolve(u,h)
    a=np.abs(z)
    if not parab: return a.max()
    k=np.argmax(a[1:-1])+1
    best=a.max()
    # refinar los 50 mayores máximos locales
    idx=np.argsort(a)[-200:]
    for k in idx:
        if k<1 or k>=len(z)-1: continue
        s=np.sign(z[k]); A,B,C=s*z[k-1],s*z[k],s*z[k+1]
        den=A-2*B+C
        if den<0:
            p=0.5*(A-C)/den; best=max(best,B-0.25*(A-C)*p)
    return best
for tpp,fp in [(12,0.42),(16,0.42),(16,0.45),(24,0.46),(32,0.47)]:
    h=make_interp(tpp,fp)
    for parab in (False,True):
        errs=[]
        for trial in range(60):
            x=rng.standard_normal(4096)
            # banda: ruido blanco limitado a 0.47 fs (como un master real) 
            X=np.fft.rfft(x); f=np.fft.rfftfreq(4096); X[f>0.475]=0; x=np.fft.irfft(X,4096)
            x=x/np.abs(x).max()*0.5
            r=ref_tp(x); e=tp_est(x,h,parab)
            errs.append(20*np.log10(e/r))
        errs=np.array(errs)
        print(f"taps/fase={tpp} fpass={fp} parábola={parab}: subestima máx {errs.min():+.3f} dB, sobreestima máx {errs.max():+.3f} dB")
