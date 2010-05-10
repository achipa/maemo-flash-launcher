cd flashlauncher
fakeroot qmake
make distclean
rm *.o -f
rm moc_* -f
rm debian/flashlauncher.substvars
cd ..
dpkg-buildpackage -rfakeroot -I.svn -S -sa
