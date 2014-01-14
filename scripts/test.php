<form method="post">
	<input type="text" name="username" placeholder="username">
	<input type="password" name="password" placeholder="password">
	<input type="submit" value="Send form!">
</form>
<?php
    echo 'PHP works evilguys<br/>';
    echo 'GET: '; var_dump($_GET);
    echo '<br/>';
    echo 'POST: '; var_dump($_POST);
    echo '<br/>';
    echo 'SERVER: '; var_dump($_SERVER); 
    echo '<br/>';
?>
