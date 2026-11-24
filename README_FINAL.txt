PPSSPP MultiSplit 1.0 — VULKAN — Windows x64

REQUISITOS: RetroArch para Windows x64 y el directorio system/PPSSPP con los
recursos del nucleo normal PPSSPP (assets). La DLL contiene el motor original.

INSTALACION
1. Copiar cores/ppsspp_multisplit_libretro.dll a RetroArch/cores.
2. Copiar info/ppsspp_multisplit_libretro.info a RetroArch/info.
3. Eliminar o apartar de RetroArch la antigua version experimental
   ppsspp_multisplit_vulkan_libretro.dll y su .info para no duplicar menus.
   NO eliminar el nucleo oficial ppsspp_libretro.dll.
4. Si quieres evitar incluso el primer aviso del Firewall al iniciar Ad Hoc,
   ejecutar UNA SOLA VEZ CONFIGURAR_FIREWALL_LOCAL.cmd y aceptar el UAC.
   Es opcional. Crea exclusivamente una regla entrante para la aplicacion
   auxiliar y conexiones DESDE 127.0.0.0/8 y ::1 (el propio ordenador).
   No desactiva el Firewall ni autoriza acceso desde Internet o la red local.
   El script debe mantenerse junto a psp_worker.exe hasta ejecutarlo.
5. Iniciar un juego y elegir la disposicion de 2, 3 o 4 jugadores.

CAMBIO ESPECIFICO DE ESTA VERSION
- La version 0.2 extraia psp_worker.exe cada vez a una carpeta temporal distinta,
  lo que hacia que Windows Firewall lo identificara como una aplicacion nueva.
- Ahora el ejecutable auxiliar permanece en
  %LOCALAPPDATA%\PPSSPP-MultiSplit\psp_worker.exe
  y no se borra al cerrar contenido. Se comprueba su contenido antes de
  actualizarlo; al ser la misma ruta, el permiso del Firewall persiste.
- Los datos de sesion, instancias, Vulkan, red, mandos, audio, selector y
  guardados mantienen la implementacion validada de 0.2. Se ha incrustado
  el mismo ejecutable psp_worker.exe (mismo hash SHA256) que en la 0.2.
- Sin ejecutar CONFIGURAR_FIREWALL_LOCAL.cmd, Windows puede solicitar una
  ultima autorizacion para la nueva RUTA estable. Cuando se permita una vez
  en dicha ruta, no tendria que volver a hacerlo en cada lanzamiento.
- La configuracion administrativa del firewall depende de las politicas de
  Windows: no se puede garantizar la ausencia del aviso en cualquier equipo.

LIMITACIONES CONOCIDAS
- Savestates, retroceso y trucos de RetroArch no implementados.
- Se usan procesos auxiliares locales para aislar completamente las PSP.
- Vulkan real por PSP; la imagen final se compone en memoria principal.
- Algunas opciones que requieren reinicio se aplican tras cerrar/reiniciar.
- Sin prueba de ejecucion en Windows de esta compilacion final: la version 0.2
  fue validada por el usuario con cuatro jugadores, Ad Hoc y 60 FPS.

CODIGO: https://github.com/hrydgard/ppsspp (motor oficial GPL-2.0+)

RECOMPILACION: ver BUILD.py. El puente se compila con Clang / lld-link para
Windows x64. Es necesario suministrar la DLL de la release para obtener el
mismo worker validado y los binarios originales de PPSSPP; el codigo fuente
del motor original esta disponible en el repositorio oficial enlazado arriba.
