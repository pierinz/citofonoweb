function oddify(){
	$('table#report tbody tr:odd').addClass('odd');
	$('table.commenti tbody tr:odd').addClass('odd');
}
	
$(document).ready(function(){
	oddify();
	
	$("a").mouseover(function(){
		$(this).find('img').css('opacity','0.7');
	});
	$("a").mouseout(function(){
		$(this).find('img').css('opacity','1');
	});
});