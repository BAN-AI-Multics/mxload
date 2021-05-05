tbl manual.tms | ditroff -t -ms | psdit > manual.psc
ditroff -t -man  mxarc.1 mxascii.1 mxforum.1 mxload.1 mxmap.1 mxmbx.1 | psdit > pages.psc
lpr -PPostScript pages.psc manual.psc
