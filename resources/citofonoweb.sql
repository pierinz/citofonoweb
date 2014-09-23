------------------------------------------
-- Default Database Structure for MYSQL
------------------------------------------

CREATE TABLE IF NOT EXISTS `acl` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `id_device` varchar(200) COLLATE utf8_unicode_ci NOT NULL,
  `user` varchar(50) COLLATE utf8_unicode_ci NOT NULL,
  `sched` text COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `id_device` (`id_device`,`user`),
  KEY `user` (`user`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

CREATE TABLE IF NOT EXISTS `devices` (
  `id_device` varchar(200) COLLATE utf8_unicode_ci NOT NULL,
  `address` varchar(15) COLLATE utf8_unicode_ci NOT NULL,
  `conffile` varchar(255) COLLATE utf8_unicode_ci NOT NULL,
  `notes` text COLLATE utf8_unicode_ci,
  PRIMARY KEY (`id_device`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

CREATE TABLE IF NOT EXISTS `profiles` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `profile` varchar(50) COLLATE utf8_unicode_ci NOT NULL,
  `id_device` varchar(200) COLLATE utf8_unicode_ci NOT NULL,
  `sched` text COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `profile` (`profile`,`id_device`),
  KEY `id_device` (`id_device`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

CREATE TABLE IF NOT EXISTS `userdata` (
  `user` varchar(50) COLLATE utf8_unicode_ci NOT NULL,
  `name` varchar(100) COLLATE utf8_unicode_ci NOT NULL,
  `notes` text COLLATE utf8_unicode_ci,
  PRIMARY KEY (`user`),
  UNIQUE KEY `name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- This is the badge table, but it is called `users` for backward compatibility
-- You can use AUTO_INCREMENT on `id` if you don't need it
CREATE TABLE IF NOT EXISTS `users` (
  `id` int(11) NOT NULL,
  `user` varchar(50) COLLATE utf8_unicode_ci NOT NULL,
  `allowed` tinyint(4) NOT NULL DEFAULT '1',
  `msr_code` varchar(25) COLLATE utf8_unicode_ci DEFAULT NULL, -- I have a mixed environment, so i save magnetic stripe and rfid code.
  `rfid_code` varchar(25) COLLATE utf8_unicode_ci DEFAULT NULL, -- You can choose one and remove the other to save space
  PRIMARY KEY (`id`),
  UNIQUE KEY `rfid_code` (`rfid_code`),
  UNIQUE KEY `msr_code` (`msr_code`),
  KEY `user` (`user`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

ALTER TABLE `acl`
  ADD CONSTRAINT `acl_ibfk_1` FOREIGN KEY (`id_device`) REFERENCES `devices` (`id_device`),
  ADD CONSTRAINT `acl_ibfk_2` FOREIGN KEY (`user`) REFERENCES `userdata` (`user`);

ALTER TABLE `profiles`
  ADD CONSTRAINT `profiles_ibfk_1` FOREIGN KEY (`id_device`) REFERENCES `devices` (`id_device`);

ALTER TABLE `users`
  ADD CONSTRAINT `users_ibfk_1` FOREIGN KEY (`user`) REFERENCES `userdata` (`user`);
