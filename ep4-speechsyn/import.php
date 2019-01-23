<?php
#define('FIXFAC', -40960);
#define('FIXFAC', -3500);
$load = Array(['a',   [10,223]     , 'choose_randomly', 1.0],
              ['e',   [/*33,*/245]     , 'choose_randomly', 1.0],
              ['i',   [/*67,*/275]     , 'choose_randomly', 1.0],
              ['o',   [/*87,*/302]     , 'choose_randomly', 1.0],
              ['u',   [/*118,*/322]    , 'choose_randomly', 1.0],
              ['y',   [148,346]    , 'choose_randomly', 1.0],
              ['ä',   [170,373]    , 'choose_randomly', 1.0],
              ['ö',   [201,398]    , 'choose_randomly', 1.0],
              ['l',   [19]         , 'choose_randomly', 1.0],
              ['v',   [47,48,49,50,51], 'play_in_sequence',1.0],
              ['j',   [77,78,79]   , 'play_in_sequence',1.0],
              ['m',   [105,108]    , 'choose_randomly', 1.0],
              ['n',   [129]        , 'choose_randomly', 1.0],
              ['ŋ',   [158]        , 'choose_randomly', 1.0],
              ['r',   [186,189]    , 'trill',           0.7],
              ['h',   [229,233,234], 'choose_randomly', 0.8],
              ['s',   [259]        , 'choose_randomly', 0.6],
              ['M',   [108,109,110], 'play_in_sequence', 1.0],
              ['N',   [136]        , 'play_in_sequence', 1.0],
              ['Ŋ',   [162,163,164,165]    , 'play_in_sequence', 1.0],
              ['k',   [/*284,285,286,287,*/291,292,293],
                                     'play_in_sequence',0.7],
              ['p',   [315,316,/*314,315,316,317*/], 'play_in_sequence',0.9],
              ['t',   [341,342,343], 'play_in_sequence',0.7],
              ['d',   [366,367,368], 'play_in_sequence',1.0],
              ['q',   [214,291]    , 'choose_randomly', 1.0],
              ['-',   [385,386]    , 'choose_randomly', 1.0]
//              ['-',   [209]        , 'choose_randomly', 1.0]
        );

function format_float($f, $maxlen)
{
  return sprintf("%{$maxlen}s", `./float_opt $f $maxlen`);

  $result    = substr($f,0,$maxlen);
  $besterror = 1e99;
  $bestmul   = 0;
  foreach(Array(1/*,2,3,4,5,6,7,8,9,11,13,17,19,21,27,31,37,43,47,'.3','.4','.6','.7','.8','.9'*/) as $divisor)
  for($delta=-0; $delta<=0; ++$delta)
  for($sub=0; $sub<2; ++$sub)
  for($inv=-9; $inv<=9; ++$inv)
  {
    if($inv < 0) $mul = -$inv;
    else         $mul = 1;

    if($sub && ($delta < 0 || $delta > 9)) continue;
    if($delta!=0 && abs($inv)>=10) continue;
    if($mul!=1 && $divisor!=1) continue;
    #if($delta||$sub||$inv||$divisor!=1)continue;

    foreach(Array('f','e') as $fmt)
      for($n=0; $n<=$maxlen; ++$n)
      {
        if($sub)
          if($inv>0)
            $s = sprintf("%.{$n}{$fmt}", $inv/($delta-$f)*$divisor/$mul);
          else
            $s = sprintf("%.{$n}{$fmt}", ($delta-$f)*$divisor/$mul);
        else
          if($inv>0)
            $s = sprintf("%.{$n}{$fmt}", $inv/($f-$delta)*$divisor/$mul);
          else
            $s = sprintf("%.{$n}{$fmt}", ($f-$delta)*$divisor/$mul);

        if(substr($s,0,2) === "0.") $s = substr($s,1);
        elseif(substr($s,0,3) === "-0.") $s = '-'.substr($s,2);
        $s = str_replace('e+', 'e', $s);
        if(strlen($s) > $maxlen) continue;
        if($inv>0)
        {
          if((float)$s == 0.) continue;
          $s = "$inv/$s";
          if(strpos($s, '.') === false
          && strpos($s, 'e') === false) $s .= '.';
          if(strlen($s) > $maxlen) continue;
        }
        if($mul>1) $s = "$s*$mul";
        if($divisor != 1)
        {
          if(strpos($s, '.') === false
          && strpos($s, 'e') === false) $s .= '.';
          $s = "$s/$divisor";
        }
        if(strlen($s) > $maxlen) continue;
        if($sub)        { $s = sprintf('%d-%s', $delta, $s); $s = str_replace('--', '+', $s); }
        else { if($delta) $s = sprintf('%s%+d', $s, $delta); }
        if(strlen($s) > $maxlen) continue;

        $p=0;
        eval("\$p=$s;");
        $error = abs($f - $p);
        #printf("%12s: error= %16.16f\n", sprintf("'%s'",$s),$error);
        if($error < $besterror
        || ($error == $besterror && (strlen($s) < strlen($result)
                                  || (strlen($s) == strlen($result) && (abs($inv)+abs($delta)) < $bestmul)))
          ) { $besterror = $error; $result = $s; $bestmul = abs($inv)+abs($delta); }
      }
  }
  return sprintf("%{$maxlen}s", $result);
}

$len = (int)$argv[1];
if(!$len) $len = 6;


$frames = Array();
$frame  = Array();
foreach(file('ahesiko44100-48b.LPC') as $line)
#foreach(file('ahesiko44100-burg32.LPC') as $line)
#foreach(file('ahesiko44100-burg128.LPC') as $line)
{
  if(sscanf($line, "            a [%d] = %f", $index, $coeff) == 2)
    $frame[] = $coeff;
  elseif(sscanf($line, "        gain = %f", $gain) == 1)
    { $frames[] = Array('f'=>$frame, 'g'=>$gain); $frame = Array(); }
}
#printf("#define FIXFAC %d\n", FIXFAC);
foreach($load as $d)
{
  $alts = $d[1];
  printf("  {U'%s', { record_modes::%s, %.1f, {\n", $d[0], $d[2], $d[3]);
  foreach($alts as $c=>$index)
  {
    if($c) print ",\n";
    $frame = $frames[$index];

    printf("    { %s,{", format_float($frame['g'],6));
    $sep = '';
    $n=0;
    foreach($frame['f'] as $f)
    {
      print $sep;
      $sep = ',';

      #$f=-$f;

      #print format_float($f,4);
      #if(++$n == 48) { $n=0; $sep = ",\n              "; }
      
      if($len >= 8)
      {
        print format_float($f,$len);
        if(++$n == 16) { $n=0; $sep = ",\n              "; }
      }
      else
      {
        print format_float($f,$len);
        if(++$n == 24) { $n=0; $sep = ",\n              "; }
      }
    }
    printf(" }}");
  }
  if(!empty($alts)) print "\n";
  printf("  } } },\n");
}
