<?php
namespace Lit\Exception;
use Symfony\Component\Process\Exception\RuntimeException;
use Lit\Shell\Process;

/**
 * Exception that is thrown when a process times out.
 *
 * @author Johannes M. Schmitt <schmittjoh@gmail.com>
 */
class ProcessTimedOutException extends RuntimeException
{
   const TYPE_GENERAL = 1;
   const TYPE_IDLE = 2;

   private $process;
   private $timeoutType;

   public function __construct(Process $process, int $timeoutType)
   {
      $this->process = $process;
      $this->timeoutType = $timeoutType;

      parent::__construct(sprintf(
         'The process "%s" exceeded the timeout of %s seconds.',
         $process->getCommandLine(),
         $this->getExceededTimeout()
      ));
   }

   public function getProcess()
   {
      return $this->process;
   }

   public function isGeneralTimeout()
   {
      return self::TYPE_GENERAL === $this->timeoutType;
   }

   public function isIdleTimeout()
   {
      return self::TYPE_IDLE === $this->timeoutType;
   }

   public function getExceededTimeout()
   {
      switch ($this->timeoutType) {
         case self::TYPE_GENERAL:
            return $this->process->getTimeout();

         case self::TYPE_IDLE:
            return $this->process->getIdleTimeout();

         default:
            throw new \LogicException(sprintf('Unknown timeout type "%d".', $this->timeoutType));
      }
   }
}
