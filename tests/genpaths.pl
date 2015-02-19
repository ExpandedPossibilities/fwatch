#!/usr/bin/perl
use strict;

sub print_test{
  my ($s, $p) = (@_);
  my $sa=$s?$s:'NONE';
  my $pa=$p?$p:'NONE';
  $p="$s/$p" unless $p=~m:^/:;
  $p=~s:/+:/:g;
  my $ap = $p;
  while(
    $ap=~s://+:/:g ||
    $ap=~s:(^|/)\.(?=/|$):\1: ||
    $ap=~s:(^|/)[^/]*/\.\.(?=/|$):\1:){}
  $ap='/'.$ap;
  while(
    $ap=~s:^(/+\.\./)+:/:){};
  $ap=~s:/+:/:g;
  $ap=~s:(.)/$:\1:;
  print "$sa\t$pa\t$ap\n";
}

sub encode {
  my ($n,@a) = @_;
  my $max = @a;
  my @o;
  while($n > 0) {
    my $r = $n % $max;
    push @o,$a[$r];
    $n -= $r;
    $n /= $max;
  }
  return @o;
}
my $abusefirst = 0;

sub gentemplates{
  my ($seed, $tries, $mod) = @_;
  my %all;
  my @l;
  foreach my $i (0..$tries) {
    my @comps = encode(($seed*$i) % $mod,'','d','.','d','..','d');
    my $p = (join '/',  @comps);
    my @versions;
    push @versions, $p;
    my $num = $p=~/d/;
    if($abusefirst) {
      for(0..$num){
        $p=~s:/d:/..d:;
        push @versions, $p;
      }
      $p = $versions[0];
      for(0..$num){
        $p=~s:d/:d../:;
        push @versions, $p;
      }
      $p = $versions[0];
      for(0..$num){
        $p=~s:d/:d./:;
        push @versions, $p;
      }
      $p = $versions[0];
      for(0..$num){
        $p=~s:/d:.d/:;
        push @versions, $p;
      }
      $p = $versions[0];
      for(0..$num){
        $p=~s:/d/:/.d./:;
        push @versions, $p;
      }
      $p = $versions[0];
      for(0..$num){
        $p=~s:/d/:/..d../:;
        push @versions, $p;
      }
    }
    foreach my $s (@versions) {
      unless($all{$s}){
        push @l, $s;
        $all{$s} = 1;
      }
    }
  }
  return @l;
}

#print join "\n", gentemplate(1234,3000,5000);
#print "\n";
#exit 1;

sub genpaths {
  my ($seed,$tries,$mod,@parts) = @_;
  my @templates = gentemplates($seed,$tries,$mod);
  for(@templates) {
    my $c = 0;
    s/d/$parts[$c++ % @parts]/ge;
  }
  return @templates;
}

#print join "\n", genpaths(123,3000,5000,qw/a b c e/);
#print "\n";
#exit 1;

print_test('/','');
print_test('','/');

foreach my $base (genpaths(123,31,703,qw(foxtrot golf hotel)),
                  genpaths(123,17,703,qw(foxtrot.. ..golf .hotel))) {
  foreach my $path (genpaths(423,73,947,qw(alpha beta gamma ...)),
                    genpaths(423,23,947,qw(alpha.. ..beta.. .gamma))) {
    print_test($base, $path);
  }
}
