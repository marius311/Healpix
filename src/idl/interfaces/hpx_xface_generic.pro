; -----------------------------------------------------------------------------
;
;  Copyright (C) 1997-2013  Krzysztof M. Gorski, Eric Hivon, Anthony J. Banday
;
;
;
;
;
;  This file is part of HEALPix.
;
;  HEALPix is free software; you can redistribute it and/or modify
;  it under the terms of the GNU General Public License as published by
;  the Free Software Foundation; either version 2 of the License, or
;  (at your option) any later version.
;
;  HEALPix is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;  GNU General Public License for more details.
;
;  You should have received a copy of the GNU General Public License
;  along with HEALPix; if not, write to the Free Software
;  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
;
;  For more information about HEALPix see http://healpix.sourceforge.net
;
; -----------------------------------------------------------------------------
pro hpx_xface_generic, fullpath, param_file, usrpath, init=init, run=run, silent=silent, clean=clean, cxx=cxx, tmpdir=tmpdir
;+
; does general operations for interface to Healpix f90 codes:
; - initialize path and temporary directory (when init is set to a structure)
; - run the code              (when run is set)
; - clean up temporary files (when clean is set)
;
; June 2007
; Feb 2008: increase stack size on Unix
; 2008-02-24: use File_Delete instead of spawn,'\rm', and path_sep
; 2008-02-28: deal with init.options
; 2008-05-30: send to /dev/null error message generated by ulimit
; 2008-07-04: set stack size to hard limit
; 2009-04-30: deal with tmpdir, using IDL_TMPDIR as default
; 2009-09-09: uses !healpix.path.* sub-structure
;-

do_init = keyword_set(init)
do_run  = keyword_set(run)
do_clean  = keyword_set(clean)
do_cxx = keyword_set(cxx)



common hpx_xface_com, tmp_head, to_remove, uid

if (do_init) then begin
; define path to executable
    defsysv, '!healpix', EXISTS = healpix_done
    if (~ healpix_done) then init_healpix

;    slash = (strupcase(!version.os_family) eq 'WINDOWS') ? '\' : '/'
    slash = path_sep()
    if (do_cxx) then begin
;         default_path = !healpix.directory+slash+'src'+slash+'cxx'+slash+'$HEALPIX_TARGET'+slash+'bin'+slash+init.exe_cxx
;         if (~file_test(default_path)) then default_path = !healpix.directory+slash+'src'+slash+'cxx'+slash+'generic_gcc'+slash+'bin'+slash+init.exe_cxx
        default_path = !healpix.path.bin.cxx+slash+init.exe_cxx
    endif else begin
;         default_path = '$HEXE'+slash+init.exe
;         if (~file_test(default_path)) then default_path =
;         !healpix.directory+slash+'bin'+slash+init.exe
        default_path = !healpix.path.bin.f90+slash+init.exe
    endelse
    fullpath = defined(usrpath) ? strtrim(usrpath,2) : default_path
    case 0 of
        strpos(fullpath,'~')*strpos(fullpath,slash)*strpos(fullpath,'$'): fullpath = expand_path(fullpath)
        strpos(fullpath,'./') : fullpath = fullpath
        else: fullpath = !healpix.directory+slash+fullpath
    endcase
    if (~file_test(fullpath,/noexpand)) then begin
        message,'Executable '+fullpath+' not found!'
    endif
    junk = where(tag_names(init) eq 'DOUBLE', nd)
    if (nd gt 0 && init.double eq 1 && ~do_cxx)       then fullpath = fullpath + ' --double '
    junk = where(tag_names(init) eq 'OPTIONS', no)
    if (no gt 0 && size(/tname,init.options) eq 'STRING')  then fullpath = fullpath + ' '+init.options+ ' '

; define temporary directory
;    tmp_dir = defined(tmpdir) ? add_prefix(tmpdir,slash,/suff) :    slash+'tmp'+slash
    tmp_dir = defined(tmpdir) ? tmpdir : getenv('IDL_TMPDIR')  ; 2009-04-30
    tmp_dir = add_prefix(tmp_dir,slash,/suff) ; make sure there is a trailing /
    stime = string(long(systime(1)*100 mod 1.e8), form='(i8.8)')
    tmp_head = tmp_dir+init.routine+'_'+stime+'_'

; define parameter file
    param_file = tmp_head+'param.txt'
    to_remove = param_file
endif
;------------------
if (do_run) then begin
    ; increase stack size to its hard limit on Unix systems
    push_stack_up = (strupcase(!version.os_family) eq 'UNIX') ? 'ulimit -s `ulimit -s -H` > /dev/null 2>&1 ; ' : ''
    command = push_stack_up + fullpath+' '+param_file
    if (keyword_set(silent)) then begin
        tmp_log_file = tmp_head+'log.txt'
        command = command + ' > ' + tmp_log_file
        to_remove = [to_remove, tmp_log_file]
    endif else begin
        print,command
;;;;        spawn,'cat '+param_file
    endelse
    spawn,/sh,command
endif

if (do_clean) then begin
;     for i=0,n_elements(to_remove)-1 do spawn,/sh,'\rm '+to_remove[i]
    file_delete,to_remove
endif


return
end
